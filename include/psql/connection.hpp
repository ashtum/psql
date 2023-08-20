#pragma once

#include <psql/error.hpp>
#include <psql/notification.hpp>
#include <psql/pipeline.hpp>
#include <psql/result.hpp>

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/post.hpp>

namespace psql
{
namespace asio = boost::asio;

template<typename Executor = asio::any_io_executor>
class basic_connection
{
  struct pgconn_deleter
  {
    void operator()(PGconn* p)
    {
      PQfinish(p);
    }
  };

  using socket_type = asio::posix::basic_stream_descriptor<Executor>;
  using wait_type   = typename socket_type::wait_type;
  using error_code  = boost::system::error_code;

  std::unique_ptr<PGconn, pgconn_deleter> conn_;
  socket_type socket_;
  asio::cancellation_signal notification_cs_;

public:
  using executor_type = Executor;

  explicit basic_connection(Executor exec)
    : socket_{ std::move(exec) }
  {
  }

  template<typename OtherExecutor>
  struct rebind_executor
  {
    using other = basic_connection<OtherExecutor>;
  };

  executor_type get_executor() noexcept
  {
    return socket_.get_executor();
  }

  template<typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_connect(std::string conninfo, CompletionToken&& token = CompletionToken{})
  {
    return asio::async_compose<CompletionToken, void(error_code)>(
      [this, conninfo = std::move(conninfo), init = true](auto& self, error_code ec = {}) mutable
      {
        if (ec)
          return self.complete(ec);

        if (std::exchange(init, false))
        {
          conn_.reset(PQconnectStart(conninfo.data()));
          socket_.assign(PQsocket(conn_.get()));

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
      token,
      socket_);
  }

  template<typename Operation, typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_exec_pipeline(Operation&& operation, CompletionToken&& token = CompletionToken{})
  {
    return asio::async_compose<CompletionToken, void(error_code, std::vector<result>)>(
      [this,
       coro      = asio::coroutine{},
       results   = std::vector<result>{},
       thrown    = false,
       index     = size_t{},
       operation = std::move(operation)](auto& self, error_code ec = {}, result result = {}) mutable
      {
        if (ec)
          return self.complete(ec, {});

        BOOST_ASIO_CORO_REENTER(coro)
        {
          if (!PQenterPipelineMode(conn_.get()))
            return self.complete(error::pq_enter_pipeline_mode_failed, {});

          {
            auto pipeline = psql::pipeline{ conn_.get() };
            try
            {
              operation(pipeline);
            }
            catch (...)
            {
              thrown = true;
              pipeline.push_query("ROLLBACK;");
            }
            results.resize(pipeline.size());
          }

          if (!PQpipelineSync(conn_.get()))
            return self.complete(error::pq_pipeline_sync_failed, {});

          BOOST_ASIO_CORO_YIELD async_flush(std::move(self));

          while (index < results.size())
          {
            BOOST_ASIO_CORO_YIELD async_receive_result(std::move(self));
            results[index] = std::move(result);
            BOOST_ASIO_CORO_YIELD async_receive_result(std::move(self));
            BOOST_ASSERT(!result);
            index++;
          }

          BOOST_ASIO_CORO_YIELD async_receive_result(std::move(self));
          BOOST_ASSERT(PQresultStatus(result.native_handle()) == PGRES_PIPELINE_SYNC);

          if (!PQexitPipelineMode(conn_.get()))
            return self.complete(error::pq_exit_pipeline_mode_failed, {});

          if (thrown)
            return self.complete(error::exception_in_pipeline_operation, {});

          auto result_ec = results.empty() ? error{} : result_status_to_error_code(results.back());
          self.complete(result_ec, std::move(results));

          notification_cs_.emit(asio::cancellation_type::terminal);
        }
      },
      token,
      socket_);
  }

  template<typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_query(std::string query, CompletionToken&& token = CompletionToken{})
  {
    return async_query(std::move(query), {}, std::forward<CompletionToken>(token));
  }

  template<typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_query(std::string query, params params, CompletionToken&& token = CompletionToken{})
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
      std::forward<CompletionToken>(token));
  }

  template<typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_prepare(std::string stmt_name, std::string query, CompletionToken&& token = CompletionToken{})
  {
    return async_generic_single_result_query(
      [this, query = std::move(query), stmt_name = std::move(stmt_name)]() -> error_code
      {
        if (!PQsendPrepare(conn_.get(), stmt_name.data(), query.data(), 0, nullptr))
          return error::pq_send_prepare_failed;
        return {};
      },
      std::forward<CompletionToken>(token));
  }

  template<typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_query_prepared(std::string stmt_name, params params, CompletionToken&& token = CompletionToken{})
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

  template<typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_describe_prepared(std::string stmt_name, CompletionToken&& token = CompletionToken{})
  {
    return async_generic_single_result_query(
      [this, stmt_name = std::move(stmt_name)]() -> error_code
      {
        if (!PQsendDescribePrepared(conn_.get(), stmt_name.data()))
          return error::pq_send_describe_prepared_failed;
        return {};
      },
      std::forward<CompletionToken>(token));
  }

  template<typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_describe_portal(std::string portal_name, CompletionToken&& token = CompletionToken{})
  {
    return async_generic_single_result_query(
      [this, portal_name = std::move(portal_name)]() -> error_code
      {
        if (!PQsendDescribePortal(conn_.get(), portal_name.data()))
          return error::pq_send_describe_portal_failed;
        return {};
      },
      std::forward<CompletionToken>(token));
  }

  template<typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_receive_notifcation(CompletionToken&& token = CompletionToken{})
  {
    return asio::async_compose<CompletionToken, void(error_code, notification)>(
      [this, coro = asio::coroutine{}, stored_notification = notification{}, needs_rescheduling = true](
        auto& self, error_code ec = {}) mutable
      {
        if (ec && !(ec == asio::error::operation_aborted && !self.cancelled()))
          return self.complete(ec, {});

        BOOST_ASIO_CORO_REENTER(coro)
        {
          for (;;)
          {
            if ((stored_notification = notification{ PQnotifies(conn_.get()) }))
            {
              if (needs_rescheduling)
              {
                BOOST_ASIO_CORO_YIELD asio::post(std::move(self));
              }

              return self.complete({}, std::move(stored_notification));
            }

            if (asio::get_associated_cancellation_slot(self).is_connected())
              asio::get_associated_cancellation_slot(self).assign([this](auto c) { notification_cs_.emit(c); });

            BOOST_ASIO_CORO_YIELD socket_.async_wait(
              wait_type::wait_read, asio::bind_cancellation_slot(notification_cs_.slot(), std::move(self)));

            if (!PQconsumeInput(conn_.get()))
              return self.complete(error::pq_consume_input_failed, {});
          }
        }
      },
      token,
      socket_);
  }

  std::string_view error_message() const noexcept
  {
    return PQerrorMessage(conn_.get());
  }

  ~basic_connection()
  {
    // PQfinish handles the closing of the socket.
    if (conn_)
      socket_.release();
  }

private:
  template<typename CompletionToken>
  auto async_flush(CompletionToken&& token)
  {
    return asio::async_compose<CompletionToken, void(error_code)>(
      [this](auto& self, std::array<std::size_t, 2> order = {}, error_code ec1 = {}, error_code ec2 = {})
      {
        if (order[0])
        {
          if (ec2)
            return self.complete(ec2);

          if (!PQconsumeInput(conn_.get()))
            return self.complete(error::pq_consume_input_failed);
        }
        else
        {
          if (ec1)
            return self.complete(ec1);

          int ret = PQflush(conn_.get());

          if (ret == -1)
            return self.complete(error::pq_flush_failed);

          if (ret == 0)
            return self.complete({});
        }

        return asio::experimental::make_parallel_group(
                 [this](auto token) { return socket_.async_wait(wait_type::wait_write, token); },
                 [this](auto token) { return socket_.async_wait(wait_type::wait_read, token); })
          .async_wait(asio::experimental::wait_for_one{}, std::move(self));
      },
      token,
      socket_);
  }

  template<typename CompletionToken>
  auto async_receive_result(CompletionToken&& token)
  {
    return asio::async_compose<CompletionToken, void(error_code, result)>(
      [this, coro = asio::coroutine{}, needs_rescheduling = true](auto& self, error_code ec = {}) mutable
      {
        if (ec)
          return self.complete(ec, {});

        BOOST_ASIO_CORO_REENTER(coro)
        {
          while (PQisBusy(conn_.get()))
          {
            needs_rescheduling = false;
            BOOST_ASIO_CORO_YIELD socket_.async_wait(wait_type::wait_read, std::move(self));
            if (!PQconsumeInput(conn_.get()))
              return self.complete(error::pq_consume_input_failed, {});
          }

          if (needs_rescheduling)
          {
            BOOST_ASIO_CORO_YIELD asio::post(std::move(self));
          }

          self.complete({}, result{ PQgetResult(conn_.get()) });
        }
      },
      token,
      socket_);
  }

  template<typename F, typename CompletionToken>
  auto async_generic_single_result_query(F&& send_fn, CompletionToken&& token)
  {
    return asio::async_compose<CompletionToken, void(error_code, result)>(
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
          BOOST_ASSERT(!result);

          auto result_ec = result_status_to_error_code(stored_result);
          self.complete(result_ec, std::move(stored_result));
          notification_cs_.emit(asio::cancellation_type::terminal);
        }
      },
      token,
      socket_);
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

using connection = basic_connection<>;
} // namespace psql
