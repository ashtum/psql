#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/generic/stream_protocol.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <libpq-fe.h>

namespace asiofiedpq
{
namespace net = boost::asio;

struct PGresultDeleter
{
    void operator()(PGresult* p)
    {
        PQclear(p);
    }
};
using result = std::unique_ptr<PGresult, PGresultDeleter>;

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
    struct PGconnDeleter
    {
        void operator()(PGconn* p)
        {
            PQfinish(p);
        }
    };

    using stream_protocol = net::generic::stream_protocol;
    using wait_type       = stream_protocol::socket::wait_type;

    std::unique_ptr<PGconn, PGconnDeleter> conn_;
    stream_protocol::socket socket_;

  public:
    connection(net::any_io_executor exec)
        : socket_{ exec }
    {
    }

    ~connection()
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
    }

    net::awaitable<void> async_exec_pipeline(const auto first, const auto last)
    {
        if (!PQenterPipelineMode(conn_.get()))
            throw make_error("PQenterPipelineMode failed");

        for (auto it{ first }; it != last; it++)
            if (!PQsendQueryParams(conn_.get(), it->command, 0, nullptr, nullptr, nullptr, nullptr, 0))
                throw make_error("PQsendQueryParams failed");

        if (!PQpipelineSync(conn_.get()))
            throw make_error("PQpipelineSync failed");

        while (PQflush(conn_.get()))
            co_await socket_.async_wait(wait_type::wait_write, net::deferred);

        for (auto it{ first }; it != last; it++)
        {
            while (PQisBusy(conn_.get()))
            {
                co_await socket_.async_wait(wait_type::wait_read, net::deferred);

                if (!PQconsumeInput(conn_.get()))
                    throw make_error("PQconsumeInput failed");
            }

            it->result.reset(PQgetResult(conn_.get()));
            PQgetResult(conn_.get());
        }

        while (PQisBusy(conn_.get()))
        {
            co_await socket_.async_wait(wait_type::wait_read, net::deferred);

            if (!PQconsumeInput(conn_.get()))
                throw make_error("PQconsumeInput failed");
        }
        while (PQgetResult(conn_.get()))
            ;

        if (!PQexitPipelineMode(conn_.get()))
            throw make_error("PQexitPipelineMode failed");
    }

    net::awaitable<result> async_query(query query)
    {

        if (!PQsendQueryParams(conn_.get(), query.command, 0, nullptr, nullptr, nullptr, nullptr, 0))
            throw make_error("PQsendQueryParams failed");

        while (PQflush(conn_.get()))
            co_await socket_.async_wait(wait_type::wait_write, net::deferred);

        while (PQisBusy(conn_.get()))
        {
            co_await socket_.async_wait(wait_type::wait_read, net::deferred);

            if (!PQconsumeInput(conn_.get()))
                throw make_error("PQconsumeInput failed");
        }

        auto* p = PQgetResult(conn_.get());

        while (PQgetResult(conn_.get()))
            ;

        co_return p;
    }

  private:
    std::runtime_error make_error(const char* msg)
    {
        return std::runtime_error{ std::string{ msg } + ", error:" + PQerrorMessage(conn_.get()) };
    };
};

} // namespace asiofiedpq
