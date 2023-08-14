#pragma once

#include <psql/detail/result_handler.hpp>
#include <psql/error.hpp>
#include <psql/params.hpp>
#include <psql/result.hpp>

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/experimental/co_composed.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/generic/stream_protocol.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <queue>

namespace psql
{
namespace asio = boost::asio;

struct pipelined
{
  std::string query;
  psql::params params;
  psql::result result;

  pipelined(std::string query)
    : query{ std::move(query) }
  {
  }

  pipelined(std::string query, psql::params params)
    : query{ std::move(query) }
    , params{ std::move(params) }
  {
  }
};

class connection
{
  struct pgconn_deleter
  {
    void operator()(PGconn* p)
    {
      PQfinish(p);
    }
  };

  using stream_protocol = asio::generic::stream_protocol;
  using wait_type       = stream_protocol::socket::wait_type;
  using error_code      = boost::system::error_code;

  static constexpr auto deferred       = asio::deferred;
  static constexpr auto deferred_tuple = asio::as_tuple(asio::deferred);

  std::unique_ptr<PGconn, pgconn_deleter> conn_;
  stream_protocol::socket socket_;
  asio::steady_timer write_cv_;
  std::queue<std::shared_ptr<detail::result_handler>> result_handlers_;

public:
  explicit connection(asio::any_io_executor exec)
    : socket_{ exec }
    , write_cv_{ exec, asio::steady_timer::time_point::max() }
  {
  }

  ~connection()
  {
    while (!result_handlers_.empty())
    {
      result_handlers_.front()->cancel();
      result_handlers_.pop();
    }

    // PQfinish handles the closing of the socket.
    if (conn_)
      socket_.release();
  }

  auto get_executor() noexcept
  {
    return socket_.get_executor();
  }

  auto async_connect(std::string conninfo, asio::completion_token_for<void(error_code)> auto&& token)
  {
    return asio::async_initiate<decltype(token), void(error_code)>(
      asio::experimental::co_composed<void(error_code)>(
        [](auto state, connection* self, std::string conninfo) -> void
        {
          state.on_cancellation_complete_with(asio::error::operation_aborted);

          self->conn_.reset(PQconnectStart(conninfo.data()));
          self->socket_.assign(asio::ip::tcp::v4(), PQsocket(self->conn_.get()));

          if (PQstatus(self->conn_.get()) == CONNECTION_BAD)
            co_return error::pq_status_failed;

          if (PQsetnonblocking(self->conn_.get(), 1))
            co_return error::pq_set_non_blocking_failed;

          PQsetNoticeProcessor(
            self->conn_.get(), +[](void*, const char*) {}, nullptr);

          for (;;)
          {
            auto ret = PQconnectPoll(self->conn_.get());

            if (ret == PGRES_POLLING_READING)
              if (auto [ec] = co_await self->socket_.async_wait(wait_type::wait_read, deferred_tuple); ec)
                co_return ec;

            if (ret == PGRES_POLLING_WRITING)
              if (auto [ec] = co_await self->socket_.async_wait(wait_type::wait_write, deferred_tuple); ec)
                co_return ec;

            if (ret == PGRES_POLLING_FAILED)
              co_return error::connection_failed;

            if (ret == PGRES_POLLING_OK)
              break;
          }

          if (!PQenterPipelineMode(self->conn_.get()))
            co_return error::pq_enter_pipeline_mode_failed;

          co_return {};
        },
        socket_),
      token,
      this,
      std::move(conninfo));
  }

  auto async_exec_pipeline(auto first, auto last, asio::completion_token_for<void(error_code)> auto&& token)
  {
    return asio::async_initiate<decltype(token), void(error_code)>(
      asio::experimental::co_composed<void(error_code)>(
        [](auto state, connection* self, auto first, auto last) -> void
        {
          state.on_cancellation_complete_with(asio::error::operation_aborted);

          for (auto it = first; it != last; it++)
          {
            if (!PQsendQueryParams(
                  self->conn_.get(),
                  it->query.data(),
                  it->params.count(),
                  it->params.types(),
                  it->params.values(),
                  it->params.lengths(),
                  it->params.formats(),
                  1))
              co_return error::pq_send_query_params_failed;
          }
          if (!PQpipelineSync(self->conn_.get()))
            co_return error::pq_pipeline_sync_failed;

          self->write_cv_.cancel_one();

          class pipeline_result_handler : public detail::result_handler
          {
            decltype(first) first_;
            decltype(last) last_;
            size_t n_dummy_{ 0 };

          public:
            pipeline_result_handler(decltype(first) first, decltype(last) last, asio::any_io_executor exec)
              : result_handler{ std::move(exec) }
              , first_{ first }
              , last_{ last }
            {
            }

            void handle(result res) override
            {
              if (n_dummy_)
              {
                if (--n_dummy_ == 0)
                  this->complete();
              }
              else
              {
                first_->result = std::move(res);
                if (++first_ == last_)
                  this->complete();
              }
            }

            void dumify()
            {
              // Swallows the remaining results without touching the iterators (which have become invalid)
              n_dummy_ = std::distance(first_, last_);
            }
          };

          auto rh = std::make_shared<pipeline_result_handler>(first, last, state.get_io_executor());
          self->result_handlers_.push(rh);

          co_await rh->async_wait(deferred_tuple);

          if (rh->is_waiting())
          {
            rh->dumify();
            co_return asio::error::operation_aborted;
          }

          if (rh->is_cancelled())
            co_return error::connection_failed;

          co_return {};
        },
        socket_),
      token,
      this,
      first,
      last);
  }

  auto async_query(std::string query, asio::completion_token_for<void(error_code, result)> auto&& token)
  {
    return async_query(std::move(query), {}, std::forward<decltype(token)>(token));
  }

  auto async_query(std::string query, params params, asio::completion_token_for<void(error_code, result)> auto&& token)
  {
    return async_generic_single_result_query(
      [this, query = std::move(query), params = std::move(params)]() -> error_code
      {
        if (!PQsendQueryParams(
              conn_.get(),
              query.data(),
              params.count(),
              params.types(),
              params.values(),
              params.lengths(),
              params.formats(),
              1))
          return error::pq_send_query_params_failed;
        return {};
      },
      std::forward<decltype(token)>(token));
  }

  auto async_prepare(
    std::string stmt_name,
    std::string query,
    asio::completion_token_for<void(error_code, result)> auto&& token)
  {
    return async_generic_single_result_query(
      [this, query = std::move(query), stmt_name = std::move(stmt_name)]() -> error_code
      {
        if (!PQsendPrepare(conn_.get(), stmt_name.data(), query.data(), 0, nullptr))
          return error::pq_send_prepare_failed;
        return {};
      },
      std::forward<decltype(token)>(token));
  }

  auto async_query_prepared(
    std::string stmt_name,
    params params,
    asio::completion_token_for<void(error_code, result)> auto&& token)
  {
    return async_generic_single_result_query(
      [this, stmt_name = std::move(stmt_name), params = std::move(params)]() -> error_code
      {
        if (!PQsendQueryPrepared(
              conn_.get(), stmt_name.data(), params.count(), params.values(), params.lengths(), params.formats(), 1))
          return error::pq_send_query_prepared_failed;
        return {};
      },
      token);
  }

  auto async_describe_prepared(std::string stmt_name, asio::completion_token_for<void(error_code, result)> auto&& token)
  {
    return async_generic_single_result_query(
      [this, stmt_name = std::move(stmt_name)]() -> error_code
      {
        if (!PQsendDescribePrepared(conn_.get(), stmt_name.data()))
          return error::pq_send_describe_prepared_failed;
        return {};
      },
      std::forward<decltype(token)>(token));
  }

  auto async_describe_portal(std::string portal_name, asio::completion_token_for<void(error_code, result)> auto&& token)
  {
    return async_generic_single_result_query(
      [this, portal_name = std::move(portal_name)]() -> error_code
      {
        if (!PQsendDescribePortal(conn_.get(), portal_name.data()))
          return error::pq_send_describe_portal_failed;
        return {};
      },
      std::forward<decltype(token)>(token));
  }

  auto async_run(asio::completion_token_for<void(error_code)> auto&& token)
  {
    return asio::async_initiate<decltype(token), void(error_code)>(
      asio::experimental::co_composed<void(error_code)>(
        [](auto state, connection* self) -> void
        {
          state.on_cancellation_complete_with(asio::error::operation_aborted);

          auto writer = [self](asio::completion_token_for<void(error_code)> auto&& token)
          {
            return asio::async_initiate<decltype(token), void(error_code)>(
              asio::experimental::co_composed<void(error_code)>(
                [](auto state, connection* self) -> void
                {
                  state.on_cancellation_complete_with(asio::error::operation_aborted);

                  for (;;)
                  {
                    if (auto [ec] = co_await self->write_cv_.async_wait(deferred_tuple);
                        ec != asio::error::operation_aborted)
                      co_return ec;

                    while (PQflush(self->conn_.get()))
                      if (auto [ec] = co_await self->socket_.async_wait(wait_type::wait_write, deferred_tuple); ec)
                        co_return ec;
                  }
                },
                self->socket_),
              token,
              self);
          };

          auto reader = [self](asio::completion_token_for<void(error_code)> auto&& token)
          {
            return asio::async_initiate<decltype(token), void(error_code)>(
              asio::experimental::co_composed<void(error_code)>(
                [](auto state, connection* self) -> void
                {
                  state.on_cancellation_complete_with(asio::error::operation_aborted);

                  for (;;)
                  {
                    while (!PQisBusy(self->conn_.get()))
                    {
                      auto res = result{ PQgetResult(self->conn_.get()) };

                      if (!res)
                      {
                        if (PQisBusy(self->conn_.get()))
                          break;

                        res = PQgetResult(self->conn_.get());
                        if (!res) // successive nulls means we have read all the inputs
                          break;
                      }

                      if (PQresultStatus(res.native_handle()) == PGRES_PIPELINE_SYNC)
                        continue;

                      assert(!self->result_handlers_.empty());

                      auto& rh = self->result_handlers_.front();
                      rh->handle(std::move(res));

                      if (rh->is_completed())
                        self->result_handlers_.pop();
                    }

                    if (auto [ec] = co_await self->socket_.async_wait(wait_type::wait_read, deferred_tuple); ec)
                      co_return ec;

                    if (!PQconsumeInput(self->conn_.get()))
                      co_return error::pq_consume_input_failed;
                  }
                },
                self->socket_),
              token,
              self);
          };

          auto [_, ec1, ec2] = co_await asio::experimental::make_parallel_group(writer, reader)
                                 .async_wait(asio::experimental::wait_for_one{}, deferred_tuple);

          while (!self->result_handlers_.empty())
          {
            self->result_handlers_.front()->cancel();
            self->result_handlers_.pop();
          }

          co_return ec1 ? ec1 : ec2;
        },
        socket_),
      token,
      this);
  }

  std::string_view error_message() const noexcept
  {
    return PQerrorMessage(conn_.get());
  }

private:
  auto async_generic_single_result_query(
    auto query_fn,
    asio::completion_token_for<void(error_code, result)> auto&& token)
  {
    return asio::async_initiate<decltype(token), void(error_code, result)>(
      asio::experimental::co_composed<void(error_code, result)>(
        [](auto state, connection* self, auto query_fn) -> void
        {
          state.on_cancellation_complete_with(asio::error::operation_aborted, nullptr);

          if (auto ec = query_fn())
            co_return { ec, nullptr };

          if (!PQpipelineSync(self->conn_.get()))
            co_return { error::pq_pipeline_sync_failed, nullptr };

          self->write_cv_.cancel_one();

          struct single_query_result_handler : detail::result_handler
          {
            result result;

            using detail::result_handler::result_handler;

            void handle(psql::result r) override
            {
              result = std::move(r);
              complete();
            }
          };

          auto rh = std::make_shared<single_query_result_handler>(state.get_io_executor());
          self->result_handlers_.push(rh);

          co_await rh->async_wait(deferred_tuple);

          if (rh->is_waiting())
            co_return { asio::error::operation_aborted, nullptr };

          if (rh->is_cancelled())
            co_return { error::connection_failed, nullptr };

          switch (PQresultStatus(rh->result.native_handle()))
          {
            case PGRES_SINGLE_TUPLE:
            case PGRES_TUPLES_OK:
            case PGRES_COMMAND_OK:
              co_return { error_code{}, std::move(rh->result) };
            case PGRES_BAD_RESPONSE:
              co_return { error::result_status_bad_response, std::move(rh->result) };
            case PGRES_EMPTY_QUERY:
              co_return { error::result_status_empty_query, std::move(rh->result) };
            case PGRES_FATAL_ERROR:
              co_return { error::result_status_fatal_error, std::move(rh->result) };
            default:
              co_return { error::result_status_unexpected, std::move(rh->result) };
          }
        },
        socket_),
      token,
      this,
      std::move(query_fn));
  }
};
} // namespace psql
