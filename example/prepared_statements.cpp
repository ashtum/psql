#include <psql/connection.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/deferred.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> async_main(std::string conninfo)
{
  auto exec = co_await asio::this_coro::executor;
  auto conn = psql::connection{ exec };

  co_await conn.async_connect(conninfo, asio::deferred);

  // Creating a prepared statement
  co_await conn.async_prepare("add_two", "SELECT $1::INT + $2::INT;", asio::deferred);

  // Example 1
  auto result = co_await conn.async_query_prepared("add_two", psql::mp(1, 2), asio::deferred);
  std::cout << as<int>(result) << std::endl;

  // Example 2
  auto results = co_await conn.async_exec_pipeline(
    [](psql::pipeline& p)
    {
      for (auto i = 0; i < 10; i++)
        p.push_query_prepared("add_two", psql::mp(1, i));
    },
    asio::deferred);

  for (const auto& result : results)
    std::cout << as<int>(result) << std::endl;
}
