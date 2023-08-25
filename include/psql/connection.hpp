#pragma once

#include <psql/detail/serialization.hpp>
#include <psql/detail/type_name_of.hpp>
#include <psql/error.hpp>
#include <psql/notification.hpp>
#include <psql/pipeline.hpp>
#include <psql/result.hpp>

#include <boost/asio/any_completion_handler.hpp>
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

  std::unique_ptr<PGconn, pgconn_deleter> pgconn_;
  socket_type socket_;
  asio::cancellation_signal notification_cs_;
  detail::oid_map oid_map_;
  std::vector<std::string_view> ud_types_names_;
  std::string buffer_;

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

  PGconn* native_handle() const noexcept
  {
    return pgconn_.get();
  }

  std::string_view error_message() const noexcept
  {
    return PQerrorMessage(pgconn_.get());
  }

  void close() const noexcept
  {
    pgconn_.release();
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
          pgconn_.reset(PQconnectStart(conninfo.data()));

          if (PQstatus(pgconn_.get()) == CONNECTION_BAD)
            return self.complete(error::pq_status_failed);

          if (PQsetnonblocking(pgconn_.get(), 1))
            return self.complete(error::pq_set_non_blocking_failed);

          PQsetNoticeProcessor(
            pgconn_.get(), +[](void*, const char*) {}, nullptr);

          socket_.assign(PQsocket(pgconn_.get()));
        }

        auto ret = PQconnectPoll(pgconn_.get());

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
       operation = std::forward<Operation>(operation)](auto& self, error_code ec = {}, result result = {}) mutable
      {
        if (ec)
          return self.complete(ec, {});

        BOOST_ASIO_CORO_REENTER(coro)
        {
          if (!PQenterPipelineMode(pgconn_.get()))
            return self.complete(error::pq_enter_pipeline_mode_failed, {});

          {
            auto pipeline = psql::pipeline{ pgconn_.get(), &oid_map_, &buffer_ };
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

          if (!PQpipelineSync(pgconn_.get()))
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

          if (!PQexitPipelineMode(pgconn_.get()))
            return self.complete(error::pq_exit_pipeline_mode_failed, {});

          if (thrown)
            return self.complete(error::exception_in_pipeline_operation, {});

          notification_cs_.emit(asio::cancellation_type::terminal);
          auto result_ec = results.empty() ? error{} : result_status_to_error_code(results.back());
          return self.complete(result_ec, std::move(results));
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

  template<typename... Ts, typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_query(std::string query, params<Ts...> params, CompletionToken&& token = CompletionToken{})
  {
    return asio::async_compose<CompletionToken, void(error_code, result)>(
      [this, coro = asio::coroutine{}, query = std::move(query), params = std::move(params)](
        auto& self, error_code ec = {}, result result = {}) mutable
      {
        BOOST_ASIO_CORO_REENTER(coro)
        {
          detail::extract_user_defined_types_names<Ts...>(ud_types_names_, oid_map_);

          if (!ud_types_names_.empty())
          {
            BOOST_ASIO_CORO_YIELD async_query_oids(std::move(self));
            if (ec)
              return self.complete(ec, {});
          }

          {
            auto [types, values, lengths, formats] = detail::serialize(oid_map_, buffer_, params);

            if (!PQsendQueryParams(
                  pgconn_.get(),
                  query.data(),
                  types.size(),
                  types.data(),
                  values.data(),
                  lengths.data(),
                  formats.data(),
                  1))
              return self.complete(error::pq_send_query_params_failed, {});
          }
          BOOST_ASIO_CORO_YIELD async_generic_single_result_query(std::move(self));
          return self.complete(ec, std::move(result));
        }
      },
      token,
      socket_);
  }

  template<typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_prepare(std::string stmt_name, std::string query, CompletionToken&& token = CompletionToken{})
  {
    return asio::async_compose<CompletionToken, void(error_code, result)>(
      [this, coro = asio::coroutine{}, query = std::move(query), stmt_name = std::move(stmt_name)](
        auto& self, error_code ec = {}, result result = {}) mutable
      {
        BOOST_ASIO_CORO_REENTER(coro)
        {
          if (!PQsendPrepare(pgconn_.get(), stmt_name.data(), query.data(), 0, nullptr))
            return self.complete(error::pq_send_prepare_failed, {});

          BOOST_ASIO_CORO_YIELD async_generic_single_result_query(std::move(self));
          return self.complete(ec, std::move(result));
        }
      },
      token,
      socket_);
  }

  template<typename... Params, typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto
  async_query_prepared(std::string stmt_name, std::tuple<Params...> params, CompletionToken&& token = CompletionToken{})
  {
    return asio::async_compose<CompletionToken, void(error_code, result)>(
      [this, coro = asio::coroutine{}, stmt_name = std::move(stmt_name), params = std::move(params)](
        auto& self, error_code ec = {}, result result = {}) mutable
      {
        BOOST_ASIO_CORO_REENTER(coro)
        {
          detail::extract_user_defined_types_names<Params...>(ud_types_names_, oid_map_);

          if (!ud_types_names_.empty())
          {
            BOOST_ASIO_CORO_YIELD async_query_oids(std::move(self));
            if (ec)
              return self.complete(ec, {});
          }

          {
            auto [types, values, lengths, formats] = detail::serialize(oid_map_, buffer_, params);

            if (!PQsendQueryPrepared(
                  pgconn_.get(), stmt_name.data(), types.size(), values.data(), lengths.data(), formats.data(), 1))
              return self.complete(error::pq_send_query_prepared_failed, {});
          }

          BOOST_ASIO_CORO_YIELD async_generic_single_result_query(std::move(self));
          return self.complete(ec, std::move(result));
        }
      },
      token,
      socket_);
  }

  template<typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_describe_prepared(std::string stmt_name, CompletionToken&& token = CompletionToken{})
  {
    return asio::async_compose<CompletionToken, void(error_code, result)>(
      [this, coro = asio::coroutine{}, stmt_name = std::move(stmt_name)](
        auto& self, error_code ec = {}, result result = {}) mutable
      {
        BOOST_ASIO_CORO_REENTER(coro)
        {
          if (!PQsendDescribePrepared(pgconn_.get(), stmt_name.data()))
            return self.complete(error::pq_send_describe_prepared_failed, {});

          BOOST_ASIO_CORO_YIELD async_generic_single_result_query(std::move(self));
          return self.complete(ec, std::move(result));
        }
      },
      token,
      socket_);
  }

  template<typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_describe_portal(std::string portal_name, CompletionToken&& token = CompletionToken{})
  {
    return asio::async_compose<CompletionToken, void(error_code, result)>(
      [this, coro = asio::coroutine{}, portal_name = std::move(portal_name)](
        auto& self, error_code ec = {}, result result = {}) mutable
      {
        BOOST_ASIO_CORO_REENTER(coro)
        {
          if (!PQsendDescribePortal(pgconn_.get(), portal_name.data()))
            return self.complete(error::pq_send_describe_portal_failed, {});

          BOOST_ASIO_CORO_YIELD async_generic_single_result_query(std::move(self));
          return self.complete(ec, std::move(result));
        }
      },
      token,
      socket_);
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
            if ((stored_notification = notification{ PQnotifies(pgconn_.get()) }))
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

            if (!PQconsumeInput(pgconn_.get()))
              return self.complete(error::pq_consume_input_failed, {});
          }
        }
      },
      token,
      socket_);
  }

  ~basic_connection()
  {
    // PQfinish handles the closing of the socket.
    if (pgconn_)
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

          if (!PQconsumeInput(pgconn_.get()))
            return self.complete(error::pq_consume_input_failed);
        }
        else
        {
          if (ec1)
            return self.complete(ec1);

          const int ret = PQflush(pgconn_.get());

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
          while (PQisBusy(pgconn_.get()))
          {
            needs_rescheduling = false;
            BOOST_ASIO_CORO_YIELD socket_.async_wait(wait_type::wait_read, std::move(self));
            if (!PQconsumeInput(pgconn_.get()))
              return self.complete(error::pq_consume_input_failed, {});
          }

          if (needs_rescheduling)
          {
            BOOST_ASIO_CORO_YIELD asio::post(socket_.get_executor(), std::move(self));
          }

          return self.complete({}, result{ PQgetResult(pgconn_.get()) });
        }
      },
      token,
      socket_);
  }

  auto async_query_oids_erased(asio::any_completion_handler<void(error_code)> handler)
  {
    return asio::async_compose<decltype(handler), void(error_code)>(
      [this, coro = asio::coroutine{}](auto& self, error_code ec = {}, result result = {}) mutable
      {
        if (ec)
          return self.complete(ec);

        BOOST_ASIO_CORO_REENTER(coro)
        {
          {
            auto [types, values, lengths, formats] = detail::serialize(oid_map_, buffer_, mp(ud_types_names_));

            if (!PQsendQueryParams(
                  pgconn_.get(),
                  "SELECT"
                  "  type_name,"
                  "  COALESCE(to_regtype(type_name)::oid, - 1),"
                  "  COALESCE(to_regtype(type_name || '[]')::oid, - 1)"
                  "FROM"
                  "  UNNEST($1) As type_name",
                  types.size(),
                  types.data(),
                  values.data(),
                  lengths.data(),
                  formats.data(),
                  1))
              return self.complete(error::pq_send_query_params_failed);
          }

          BOOST_ASIO_CORO_YIELD async_generic_single_result_query(std::move(self));

          if (auto ec = result_status_to_error_code(result))
            return self.complete(ec);

          for (auto row : result)
          {
            auto [type_name, type_oid, array_oid] = as<std::string_view, uint32_t, uint32_t>(row);

            if (type_oid == 0xFFFFFFFF || array_oid == 0xFFFFFFFF)
              return self.complete(error::user_defined_type_does_not_exist);

            oid_map_.emplace(type_name, detail::oid_pair{ type_oid, array_oid });
          }

          return self.complete({});
        }
      },
      handler,
      socket_);
  }

  template<typename CompletionToken>
  auto async_query_oids(CompletionToken&& token)
  {
    return asio::async_initiate<CompletionToken, void(error_code)>(
      [this](auto handler) { async_query_oids_erased(std::move(handler)); }, token);
  }

  void async_generic_single_result_query_erased(asio::any_completion_handler<void(error_code, result)> handler)
  {
    return asio::async_compose<decltype(handler), void(error_code, result)>(
      [this, coro = asio::coroutine{}, stored_result = result{}](
        auto& self, error_code ec = {}, result result = {}) mutable
      {
        if (ec)
          return self.complete(ec, {});

        BOOST_ASIO_CORO_REENTER(coro)
        {
          BOOST_ASIO_CORO_YIELD async_flush(std::move(self));

          BOOST_ASIO_CORO_YIELD async_receive_result(std::move(self));
          stored_result = std::move(result);

          BOOST_ASIO_CORO_YIELD async_receive_result(std::move(self));
          BOOST_ASSERT(!result);

          notification_cs_.emit(asio::cancellation_type::terminal);
          auto result_ec = result_status_to_error_code(stored_result);
          return self.complete(result_ec, std::move(stored_result));
        }
      },
      handler,
      socket_);
  }

  template<typename CompletionToken>
  auto async_generic_single_result_query(CompletionToken&& token)
  {
    return asio::async_initiate<CompletionToken, void(error_code, result)>(
      [this](auto handler) { async_generic_single_result_query_erased(std::move(handler)); }, token);
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
