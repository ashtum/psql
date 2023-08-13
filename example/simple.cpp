#include <asiofiedpq/connection.hpp>
#include <asiofiedpq/detail/deserialization.hpp>

#include <boost/asio/awaitable.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> run_exmaple(asiofiedpq::connection& conn)
{
  // Example 1
  auto [a, b] = co_await conn.async_query<std::string, int>("SELECT 'one'::TEXT, 2;", asio::deferred);
  std::cout << a << "-" << b << std::endl;

  // Example 2
  co_await conn.async_query("CREATE TABLE IF NOT EXISTS actors (name TEXT, age INT);", asio::deferred);
  co_await conn.async_query("INSERT INTO actors VALUES ($1, $2);", { "Bruce Lee", 32 }, asio::deferred);
  co_await conn.async_query("INSERT INTO actors VALUES ($1, $2);", { "Jackie Chan", 70 }, asio::deferred);
  auto actors = co_await conn.async_query("SELECT name, age from actors", asio::deferred);
  for (const auto row : actors)
  {
    const auto [name, age] = row.as<std::string_view, int>();
    std::cout << name << ": " << age << std::endl;

    // Alternatively:
    // std::cout << row.at("name").as<std::string_view>() << ": " << row.at("age").as<int>() << std::endl;
    // or:
    // std::cout << row.at(0).as<std::string_view>() << ": " << row.at(1).as<int>() << std::endl;
  }

  // Example 3
  auto result = co_await conn.async_query("SELECT $1 as array;", std::vector{ "1", "2", "3" }, asio::deferred);
  for (const auto value : result.at(0).at("array").as<std::vector<std::string_view>>())
    std::cout << value << ' ' << std::endl;
}
