#include <psql/connection.hpp>
#include <psql/detail/deserialization.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/deferred.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> run_exmaple(psql::connection& conn)
{
  // Example 1
  auto [a, b] = as<std::string, int>(co_await conn.async_query("SELECT 'one'::TEXT, 2;", asio::deferred));
  std::cout << a << "-" << b << std::endl;

  // Example 2
  co_await conn.async_query("DROP TABLE IF EXISTS actors;", asio::deferred);
  co_await conn.async_query("CREATE TABLE actors (name TEXT, age INT);", asio::deferred);
  co_await conn.async_query("INSERT INTO actors VALUES ($1, $2);", { "Bruce Lee", 32 }, asio::deferred);
  co_await conn.async_query("INSERT INTO actors VALUES ($1, $2);", { "Jackie Chan", 70 }, asio::deferred);
  auto actors = co_await conn.async_query("SELECT name, age from actors", asio::deferred);
  for (const auto row : actors)
  {
    const auto [name, age] = as<std::string_view, int>(row);
    std::cout << name << ": " << age << std::endl;

    // Alternatively:
    // std::cout << as<std::string_view>(row.at("name")) << ": " << as<int>(row.at("age")) << std::endl;
    // or:
    // std::cout << as<std::string_view>(row.at(0)) << ": " << as<int>(row.at(1)) << std::endl;
  }

  // Example 3
  auto result = co_await conn.async_query("SELECT $1 as array;", std::vector{ "1", "2", "3" }, asio::deferred);
  for (const auto value : as<std::vector<std::string_view>>(result))
    std::cout << value << ' ';
  std::cout << std::endl;
}
