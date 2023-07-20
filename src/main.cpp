#include "pipelined.hpp"

#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/this_coro.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> phonebook_demo(asiofiedpq::pipelined& conn)
{
    conn.send_query_params("DROP TABLE IF EXISTS phonebook;");

    conn.send_query_params("CREATE TABLE phonebook(phone VARCHAR, name VARCHAR, address VARCHAR);");

    conn.send_query_params("INSERT INTO phonebook(phone, name, address) VALUES"
                           "('+1 111 444 7777', 'Jake Harrison', 'North America'),"
                           "('+2 333 222 3333', 'Megan Gordon', 'Italy');");

    auto phonebook = conn.send_query_params("SELECT * FROM phonebook ORDER BY name;");
    auto timestamp = conn.send_query_params("SELECT NOW()::timestamp(0);");

    conn.send_sync();

    co_await phonebook.async_wait(asio::deferred);
    auto result = phonebook.get().get();

    for (auto i = 0; i < PQntuples(result); i++)
    {
        for (auto j = 0; j < PQnfields(result); j++)
            std::cout << PQfname(result, j) << ":" << PQgetvalue(result, i, j) << '\t';
        std::cout << std::endl;
    }

    co_await timestamp.async_wait(asio::deferred);
    std::cout << PQgetvalue(timestamp.get().get(), 0, 0) << std::endl;
}

asio::awaitable<void> async_main()
{
    using namespace asio::experimental::awaitable_operators;

    auto exec = co_await asio::this_coro::executor;
    auto conn = asiofiedpq::pipelined{ exec };

    co_await conn.async_connect("postgresql://postgres:postgres@172.18.0.2:5432");

    co_await (conn.async_run() || phonebook_demo(conn));
}

int main()
{
    try
    {
        asio::io_context ioc{};

        asio::co_spawn(
            ioc,
            async_main(),
            [](std::exception_ptr ep)
            {
                if (ep)
                    std::rethrow_exception(ep);
            });

        ioc.run();
    }
    catch (const std::exception& e)
    {
        std::cout << "exception:" << e.what() << std::endl;
    }
}
