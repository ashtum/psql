#pragma once

#include <oneshot.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/generic/stream_protocol.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>

#include <queue>
#include <string_view>

#include <libpq-fe.h>

namespace asiofiedpq
{
namespace net = boost::asio;

class pipelined
{
    struct PGconnDeleter
    {
        void operator()(PGconn* p)
        {
            PQfinish(p);
        }
    };

    struct PGresultDeleter
    {
        void operator()(PGresult* p)
        {
            PQclear(p);
        }
    };

    using stream_protocol = net::generic::stream_protocol;
    using wait_type       = stream_protocol::socket::wait_type;
    using result_t        = std::unique_ptr<PGresult, PGresultDeleter>;

    std::unique_ptr<PGconn, PGconnDeleter> conn_;
    stream_protocol::socket socket_;
    std::queue<oneshot::sender<result_t>> queue_;
    net::steady_timer write_cv_;

  public:
    pipelined(net::any_io_executor exec)
        : socket_{ exec }
        , write_cv_{ exec, net::steady_timer::time_point::max() }
    {
    }

    ~pipelined()
    {
        // PQfinish handles the closing of the socket.
        socket_.release();
    }

    net::awaitable<void> async_connect(std::string connection_info)
    {
        conn_.reset(PQconnectStart(connection_info.data()));

        if (PQstatus(conn_.get()) == CONNECTION_BAD)
            throw make_error("PQconnectStart failed");

        if (PQsetnonblocking(conn_.get(), 1))
            throw make_error("PQsetnonblocking failed");

        PQsetNoticeProcessor(
            conn_.get(), +[](void*, const char*) {}, nullptr);

        socket_.assign(net::ip::tcp::v4(), PQsocket(conn_.get()));

        for (;;)
        {
            auto ret = PQconnectPoll(conn_.get());

            if (ret == PGRES_POLLING_READING)
                co_await socket_.async_wait(wait_type::wait_read, net::deferred);

            if (ret == PGRES_POLLING_WRITING)
                co_await socket_.async_wait(wait_type::wait_write, net::deferred);

            if (ret == PGRES_POLLING_FAILED)
                throw make_error("PQconnectPoll failed");

            if (ret == PGRES_POLLING_OK)
                break;
        }

        if (!PQenterPipelineMode(conn_.get()))
            throw make_error("PQenterPipelineMode failed");
    }

    oneshot::receiver<result_t> send_query_params(const char* command)
    {
        if (!PQsendQueryParams(conn_.get(), command, 0, nullptr, nullptr, nullptr, nullptr, 0))
            throw make_error("PQsendQuery failed");

        write_cv_.cancel_one();

        auto [sender, receiver] = oneshot::create<result_t>();
        queue_.push(std::move(sender));

        return std::move(receiver);
    }

    void send_sync()
    {
        if (!PQpipelineSync(conn_.get()))
            throw make_error("PQpipelineSync failed");

        write_cv_.cancel_one();
    }

    net::awaitable<void> async_run()
    {
        auto writer = [&]() -> net::awaitable<void>
        {
            const auto cs = co_await net::this_coro::cancellation_state;
            for (;;)
            {
                auto [ec] = co_await write_cv_.async_wait(net::as_tuple(net::deferred));
                if (ec != net::error::operation_aborted || !!cs.cancelled())
                    co_return;

                while (PQflush(conn_.get()))
                    co_await socket_.async_wait(wait_type::wait_write, net::deferred);
            }
        };

        auto reader = [&]() -> net::awaitable<void>
        {
            for (;;)
            {
                while (!PQisBusy(conn_.get()))
                {
                    auto res = std::unique_ptr<PGresult, PGresultDeleter>{ PQgetResult(conn_.get()) };

                    if (!res)
                    {
                        res.reset(PQgetResult(conn_.get()));
                        if (!res)
                            break;
                    }

                    if (PQresultStatus(res.get()) == PGRES_PIPELINE_SYNC)
                        continue;

                    assert(!queue_.empty());

                    queue_.front().send(std::move(res));
                    queue_.pop();
                }

                co_await socket_.async_wait(wait_type::wait_read, net::deferred);

                if (!PQconsumeInput(conn_.get()))
                    throw make_error("PQconsumeInput failed");
            }
        };

        co_await net::experimental::make_parallel_group(
            net::co_spawn(socket_.get_executor(), writer, net::deferred),
            net::co_spawn(socket_.get_executor(), reader, net::deferred))
            .async_wait(net::experimental::wait_for_one(), net::deferred);
    }

  private:
    std::runtime_error make_error(std::string_view message)
    {
        return std::runtime_error{ std::string{ message } + ", error:" + PQerrorMessage(conn_.get()) };
    };
};

} // namespace asiofiedpq
