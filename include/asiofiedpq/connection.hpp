#pragma once

#include "error.hpp"
#include "result.hpp"

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/co_composed.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/generic/stream_protocol.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <oneshot.hpp>

#include <libpq-fe.h>

#include <queue>

namespace asiofiedpq
{
namespace asio = boost::asio;

struct query
{
  const char* command;

  query(const char* command)
    : command{ command }
  {
  }
};

struct pipelined_query
{
  const char* command;
  asiofiedpq::result result;

  pipelined_query(const char* command)
    : command{ command }
  {
  }
};

class connection
{
  struct result_handler
  {
    virtual bool operator()(result) = 0;
    virtual ~result_handler()       = default;
  };

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

  static constexpr auto deferred_tuple{ asio::as_tuple(asio::deferred) };

  std::unique_ptr<PGconn, pgconn_deleter> conn_;
  stream_protocol::socket socket_;
  asio::steady_timer write_cv_;
  std::queue<std::unique_ptr<result_handler>> result_handlers_;

public:
  explicit connection(asio::any_io_executor exec)
    : socket_{ exec }
    , write_cv_{ exec, asio::steady_timer::time_point::max() }
  {
  }

  ~connection()
  {
    // PQfinish handles the closing of the socket.
    if (conn_)
      socket_.release();
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
            co_return error::pqstatus_failed;

          if (PQsetnonblocking(self->conn_.get(), 1))
            co_return error::pqsetnonblocking_failed;

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
            co_return error::pqenterpipelinemode_failed;

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
            if (!PQsendQueryParams(self->conn_.get(), it->command, 0, nullptr, nullptr, nullptr, nullptr, 0))
              co_return error::pqsendqueryparams_failed;

          if (!PQpipelineSync(self->conn_.get()))
            co_return error::pqpipelinesync_failed;

          self->write_cv_.cancel_one();

          auto [sender, receiver] = oneshot::create<void>();

          // TODO Change result_handler state on cancellation
          struct pipeline_result_handler : result_handler
          {
            decltype(first) first_;
            decltype(last) last_;
            oneshot::sender<void> sender_;

            pipeline_result_handler(decltype(first) first, decltype(last) last, oneshot::sender<void> sender)
              : first_{ first }
              , last_{ last }
              , sender_{ std::move(sender) }
            {
            }

            bool operator()(result res) override
            {
              first_->result = std::move(res);

              if (++first_ == last_)
              {
                sender_.send();
                return true;
              }

              return false;
            }
          };

          self->result_handlers_.push(std::make_unique<pipeline_result_handler>(first, last, std::move(sender)));

          if (auto [ec] = co_await receiver.async_wait(deferred_tuple); ec)
          {
            if (!!state.cancelled())
              co_return ec;

            co_return error::connection_failed;
          }

          co_return {};
        },
        socket_),
      token,
      this,
      first,
      last);
  }

  auto async_query(query query, asio::completion_token_for<void(error_code, result)> auto&& token)
  {
    return asio::async_initiate<decltype(token), void(error_code, result)>(
      asio::experimental::co_composed<void(error_code, result)>(
        [](auto state, connection* self, struct query query) -> void
        {
          state.on_cancellation_complete_with(asio::error::operation_aborted, nullptr);

          if (!PQsendQueryParams(self->conn_.get(), query.command, 0, nullptr, nullptr, nullptr, nullptr, 0))
            co_return { error::pqsendqueryparams_failed, nullptr };

          if (!PQpipelineSync(self->conn_.get()))
            co_return { error::pqpipelinesync_failed, nullptr };

          self->write_cv_.cancel_one();

          auto [sender, receiver] = oneshot::create<result>();

          // TODO Change result_handler state on cancellation
          struct single_query_result_handler : result_handler
          {
            oneshot::sender<result> sender_;

            explicit single_query_result_handler(oneshot::sender<result> sender)
              : sender_{ std::move(sender) }
            {
            }

            bool operator()(result res) override
            {
              sender_.send(std::move(res));
              return true;
            }
          };

          self->result_handlers_.push(std::make_unique<single_query_result_handler>(std::move(sender)));

          if (auto [ec] = co_await receiver.async_wait(deferred_tuple); ec)
          {
            if (!!state.cancelled())
              co_return { ec, nullptr };

            co_return { error::connection_failed, nullptr };
          }

          co_return { error_code{}, std::move(receiver.get()) };
        },
        socket_),
      token,
      this,
      std::move(query));
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
                    auto [ec] = co_await self->write_cv_.async_wait(asio::as_tuple(asio::deferred));
                    if (ec != asio::error::operation_aborted || !!state.get_cancellation_state().cancelled())
                      break;

                    while (PQflush(self->conn_.get()))
                      if (auto [ec] = co_await self->socket_.async_wait(wait_type::wait_write, deferred_tuple); ec)
                        co_return ec;
                  }

                  co_return {};
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

                        res.reset(PQgetResult(self->conn_.get()));
                        if (!res) // successive nulls means we have read all the inputs
                          break;
                      }

                      if (PQresultStatus(res.get()) == PGRES_PIPELINE_SYNC)
                        continue;

                      assert(!self->result_handlers_.empty());

                      if (self->result_handlers_.front()->operator()(std::move(res)))
                        self->result_handlers_.pop();
                    }

                    if (auto [ec] = co_await self->socket_.async_wait(wait_type::wait_read, deferred_tuple); ec)
                      co_return ec;

                    if (!PQconsumeInput(self->conn_.get()))
                      co_return error::pqconsumeinput_failed;
                  }

                  co_return {};
                },
                self->socket_),
              token,
              self);
          };

          auto [_, ec1, ec2] = co_await asio::experimental::make_parallel_group(writer, reader)
                                 .async_wait(asio::experimental::wait_for_one{}, deferred_tuple);

          // TODO Drain the result handlers

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
};

} // namespace asiofiedpq
