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

  // Example 1
  // Here we use std::string for deserializing the first field because in this scenario, the result of async_query would be a
  // temporary object. If we had used std::string_view, it could point to a destroyed buffer.
  auto [a, b] = as<std::string, int>(co_await conn.async_query("SELECT 'one'::TEXT, 2;", asio::deferred));
  std::cout << a << "-" << b << std::endl;

  // Example 2
  co_await conn.async_query("DROP TABLE IF EXISTS actors;", asio::deferred);
  co_await conn.async_query("CREATE TABLE actors (name TEXT, age INT);", asio::deferred);
  co_await conn.async_query("INSERT INTO actors VALUES ($1, $2);", psql::mp("Bruce Lee", 32), asio::deferred);
  co_await conn.async_query("INSERT INTO actors VALUES ($1, $2);", psql::mp("Brad Pitt", 59), asio::deferred);

  auto actors = co_await conn.async_query("SELECT name, age FROM actors", asio::deferred);
  for (const auto row : actors)
  {
    // Because result is preserved in the `actors` variable, we can use std::string_view for accessing TEXT fields.
    const auto [name, age] = as<std::string_view, int>(row);
    std::cout << name << ": " << age << std::endl;

    // Alternatively:
    // std::cout << as<std::string_view>(row.at("name")) << ": " << as<int>(row.at("age")) << std::endl;
    // or:
    // std::cout << as<std::string_view>(row.at(0)) << ": " << as<int>(row.at(1)) << std::endl;
  }

  // Example 3
  // PostgreSQL permits field types to be arrays, and psql also accepts array types in the parameter list (currently
  // only supports one dimension).
  auto result = co_await conn.async_query("SELECT $1;", psql::mp(std::array{ "1", "2", "3" }), asio::deferred);

  // We can deserialize array fields into sequential containers.
  for (const auto value : as<std::vector<std::string_view>>(result))
    std::cout << value << ' ';
  std::cout << std::endl;
}
