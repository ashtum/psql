#include "connection.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/this_coro.hpp>

#include <iostream>
#include <vector>

namespace asio = boost::asio;

asio::awaitable<void> async_main()
{
    auto conn     = asiofiedpq::connection{ co_await asio::this_coro::executor };
    auto pipeline = std::vector<asiofiedpq::pipelined_query>{};

    co_await conn.async_connect("postgresql://postgres:postgres@172.18.0.2:5432");

    pipeline.push_back("DROP TABLE IF EXISTS phonebook;");
    pipeline.push_back("CREATE TABLE phonebook(phone VARCHAR, name VARCHAR);");
    pipeline.push_back("INSERT INTO phonebook VALUES ('+1 111 444 7777', 'Jake'),('+2 333 222 3333', 'Megan');");
    pipeline.push_back("SELECT * FROM phonebook ORDER BY name;");

    co_await conn.async_exec_pipeline(pipeline.begin(), pipeline.end());

    PGresult* phonebook = pipeline.at(3).result.get();
    for (auto i = 0; i < PQntuples(phonebook); i++)
    {
        for (auto j = 0; j < PQnfields(phonebook); j++)
            std::cout << PQfname(phonebook, j) << ":" << PQgetvalue(phonebook, i, j) << '\t';
        std::cout << std::endl;
    }

    auto timestamp = co_await conn.async_query("SELECT NOW()::timestamp(0);");
    std::cout << PQgetvalue(timestamp.get(), 0, 0) << std::endl;
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
