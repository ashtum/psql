#pragma once

#include <psql/error.hpp>
#include <psql/notification.hpp>
#include <psql/pipeline.hpp>
#include <psql/result.hpp>

#include <boost/asio/associated_immediate_executor.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/generic/stream_protocol.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace psql
{
namespace asio = boost::asio;

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

  std::unique_ptr<PGconn, pgconn_deleter> conn_;
  stream_protocol::socket socket_;
  asio::steady_timer shared_wait_cv_;
  bool is_waiting_{ false };

public:
  explicit connection(asio::any_io_executor exec)
    : socket_{ exec }
    , shared_wait_cv_{ std::move(exec), asio::steady_timer::time_point::max() }
  {
  }

  ~connection()
  {
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
    return asio::async_compose<decltype(token), void(error_code)>(
      [this, conninfo = std::move(conninfo), init = true](auto& self, error_code ec = {}) mutable
      {
        if (ec)
          return self.complete(ec);

        if (std::exchange(init, false))
        {
          conn_.reset(PQconnectStart(conninfo.data()));
          socket_.assign(asio::ip::tcp::v4(), PQsocket(conn_.get()));

          if (PQstatus(conn_.get()) == CONNECTION_BAD)
            return self.complete(error::pq_status_failed);

          if (PQsetnonblocking(conn_.get(), 1))
            return self.complete(error::pq_set_non_blocking_failed);

          PQsetNoticeProcessor(
            conn_.get(), +[](void*, const char*) {}, nullptr);
        }

        auto ret = PQconnectPoll(conn_.get());

        if (ret == PGRES_POLLING_READING)
          return socket_.async_wait(wait_type::wait_read, std::move(self));

        if (ret == PGRES_POLLING_WRITING)
          return socket_.async_wait(wait_type::wait_write, std::move(self));

        if (ret == PGRES_POLLING_FAILED)
          return self.complete(error::connection_failed);

        return self.complete({});
      },
      token);
  }

  auto async_exec_pipeline(pipeline& pl, asio::completion_token_for<void(error_code, std::vector<result>)> auto&& token)
  {
    return async_exec_pipeline_impl(pl, std::forward<decltype(token)>(token));
  }

  auto async_query(std::string query, asio::completion_token_for<void(error_code, result)> auto&& token)
  {
    return async_query(std::move(query), {}, std::forward<decltype(token)>(token));
  }

  auto async_query(std::string query, params params, asio::completion_token_for<void(error_code, result)> auto&& token)
  {
    return async_single_result_impl(
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
    return async_single_result_impl(
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
    return async_single_result_impl(
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
    return async_single_result_impl(
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
    return async_single_result_impl(
      [this, portal_name = std::move(portal_name)]() -> error_code
      {
        if (!PQsendDescribePortal(conn_.get(), portal_name.data()))
          return error::pq_send_describe_portal_failed;
        return {};
      },
      std::forward<decltype(token)>(token));
  }

  auto async_receive_notifcation(asio::completion_token_for<void(error_code, notification)> auto&& token)
  {
    return asio::async_compose<decltype(token), void(error_code, notification)>(
      [this, stored_notification = notification{}, needs_rescheduling = true](auto& self, error_code ec = {}) mutable
      {
        if (ec)
          return self.complete(ec, {});

        if (stored_notification)
          return self.complete({}, notification{ std::move(stored_notification) });

        if (!PQconsumeInput(conn_.get()))
          return self.complete(error::pq_consume_input_failed, {});

        if ((stored_notification = notification{ PQnotifies(conn_.get()) }))
        {
          if (needs_rescheduling)
            return asio::post(std::move(self));

          return self.complete({}, notification{ std::move(stored_notification) });
        }

        needs_rescheduling = false;
        async_shared_wait(std::move(self));
      },
      token);
  }

  std::string_view error_message() const noexcept
  {
    return PQerrorMessage(conn_.get());
  }

private:
  auto async_shared_wait(asio::completion_token_for<void(error_code)> auto&& token)
  {
    return asio::async_compose<decltype(token), void(error_code)>(
      [this, coro = asio::coroutine{}](auto& self, error_code ec = {}) mutable
      {
        if (ec && !!self.cancelled())
          return self.complete(ec);

        BOOST_ASIO_CORO_REENTER(coro)
        {
          if (std::exchange(is_waiting_, true))
          {
            BOOST_ASIO_CORO_YIELD shared_wait_cv_.async_wait(std::move(self));
          }
          else
          {
            BOOST_ASIO_CORO_YIELD socket_.async_wait(wait_type::wait_write, std::move(self));
            shared_wait_cv_.cancel();
            is_waiting_ = false;
          }
          self.complete({});
        }
      },
      token);
  }

  auto async_flush(asio::completion_token_for<void(error_code)> auto&& token)
  {
    return asio::async_compose<decltype(token), void(error_code)>(
      [this](auto& self, error_code ec = {})
      {
        if (ec)
          return self.complete(ec);

        int ret = PQflush(conn_.get());

        if (ret == 0)
          return self.complete({});

        if (ret == 1)
          return socket_.async_wait(wait_type::wait_write, std::move(self));

        self.complete(error::pq_flush_failed);
      },
      token);
  }

  auto async_receive_result(asio::completion_token_for<void(error_code, result)> auto&& token)
  {
    return asio::async_compose<decltype(token), void(error_code, result)>(
      [this, needs_rescheduling = true](auto& self, error_code ec = {}) mutable
      {
        if (ec)
          return self.complete(ec, {});

        if (!PQconsumeInput(conn_.get()))
          return self.complete(error::pq_consume_input_failed, {});

        if (PQisBusy(conn_.get()))
        {
          needs_rescheduling = false;
          return async_shared_wait(std::move(self));
        }

        if (std::exchange(needs_rescheduling, false))
          return asio::post(std::move(self));

        self.complete({}, result{ PQgetResult(conn_.get()) });
      },
      token);
  }

  auto async_single_result_impl(auto send_fn, asio::completion_token_for<void(error_code, result)> auto&& token)
  {
    return asio::async_compose<decltype(token), void(error_code, result)>(
      [this, coro = asio::coroutine{}, stored_result = result{}, send_fn = std::move(send_fn)](
        auto& self, error_code ec = {}, result result = {}) mutable
      {
        if (ec)
          return self.complete(ec, {});

        void(this); // supress false warning "lambda capture 'this' is not used"

        BOOST_ASIO_CORO_REENTER(coro)
        {
          if (auto ec = send_fn())
            return self.complete(ec, {});

          BOOST_ASIO_CORO_YIELD async_flush(std::move(self));

          BOOST_ASIO_CORO_YIELD async_receive_result(std::move(self));
          stored_result = std::move(result);

          BOOST_ASIO_CORO_YIELD async_receive_result(std::move(self));
          self.complete(result_status_to_error_code(stored_result), std::move(stored_result));
        }
      },
      token);
  }

  auto async_exec_pipeline_impl(
    pipeline& pl,
    asio::completion_token_for<void(error_code, std::vector<result>)> auto&& token)
  {
    return asio::async_compose<decltype(token), void(error_code, std::vector<result>)>(
      [this,
       coro    = asio::coroutine{},
       results = std::vector<result>{ pl.operations.size() },
       index   = size_t{},
       pl      = &pl](auto& self, error_code ec = {}, result result = {}) mutable
      {
        if (ec)
          return self.complete(ec, {});

        BOOST_ASIO_CORO_REENTER(coro)
        {
          if (!PQenterPipelineMode(conn_.get()))
            return self.complete(error::pq_enter_pipeline_mode_failed, {});

          for (const auto& op : pl->operations)
          {
            switch (op.type)
            {
              case pipeline::type::query:
                if (!PQsendQueryParams(
                      conn_.get(),
                      op.query.data(),
                      op.params.count(),
                      op.params.types(),
                      op.params.values(),
                      op.params.lengths(),
                      op.params.formats(),
                      1))
                  return self.complete(error::pq_send_query_params_failed, {});
                break;
              case pipeline::type::prepare:
                if (!PQsendPrepare(conn_.get(), op.stmt_name.data(), op.query.data(), 0, nullptr))
                  return self.complete(error::pq_send_prepare_failed, {});
                break;
              case pipeline::type::query_prepared:
                if (!PQsendQueryPrepared(
                      conn_.get(),
                      op.stmt_name.data(),
                      op.params.count(),
                      op.params.values(),
                      op.params.lengths(),
                      op.params.formats(),
                      1))
                  return self.complete(error::pq_send_query_prepared_failed, {});
                break;
            }
          }

          if (!PQpipelineSync(conn_.get()))
            return self.complete(error::pq_pipeline_sync_failed, {});

          BOOST_ASIO_CORO_YIELD async_flush(std::move(self));

          while (index < results.size())
          {
            BOOST_ASIO_CORO_YIELD async_receive_result(std::move(self));
            results[index] = std::move(result);
            BOOST_ASIO_CORO_YIELD async_receive_result(std::move(self));
            index++;
          }

          BOOST_ASIO_CORO_YIELD async_receive_result(std::move(self));

          if (!PQexitPipelineMode(conn_.get()))
            return self.complete(error::pq_exit_pipeline_mode_failed, {});

          self.complete(result_status_to_error_code(results.back()), std::move(results));
        }
      },
      token);
  }

  static error_code result_status_to_error_code(const result& result) noexcept
  {
    switch (PQresultStatus(result.native_handle()))
    {
      case PGRES_SINGLE_TUPLE:
      case PGRES_TUPLES_OK:
      case PGRES_COMMAND_OK:
        return {};
      case PGRES_BAD_RESPONSE:
        return error::result_status_bad_response;
      case PGRES_EMPTY_QUERY:
        return error::result_status_empty_query;
      case PGRES_FATAL_ERROR:
        return error::result_status_fatal_error;
      case PGRES_PIPELINE_ABORTED:
        return error::result_status_pipeline_aborted;
      default:
        return error::result_status_unexpected;
    }
  }
};
} // namespace psql
