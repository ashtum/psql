#include <asiofiedpq/connection.hpp>
#include <asiofiedpq/detail/deserialization.hpp>

#include <boost/asio/awaitable.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> run_exmaple(asiofiedpq::connection& conn)
{
  auto r1 = co_await conn.async_query("SELECT $1 as a, $2 as b, $3 as c;", { "one", 2, 3.5 }, asio::deferred);

  const auto [a, b, c] = r1.at(0).as<std::string_view, int, double>();
  std::cout << a << "-" << b << "-" << c << std::endl;

  auto r2 = co_await conn.async_query("SELECT $1 as array;", std::vector{ "1", "2", "3" }, asio::deferred);
  for (const auto& value : r2.at(0).at("array").as<std::vector<std::string_view>>())
    std::cout << value << std::endl;

  auto r3    = co_await conn.async_query("SELECT (1, 2, '3456'::TEXT) as record;", asio::deferred);
  auto tuple = r3.at(0).at("record").as<std::tuple<int, int, std::string_view>>();
  std::cout << std::get<0>(tuple) << std::get<1>(tuple) << std::get<2>(tuple) << std::endl;

  // print(co_await conn.async_query("SELECT $1 as bool_array;", std::vector{ true, false, true }, asio::deferred));

  // print(co_await conn.async_query("SELECT NOW()::timestamp(0) as timestamp;", asio::deferred));
}
