#include <asiofiedpq/connection.hpp>
#include <asiofiedpq/detail/deserialization.hpp>

#include <boost/asio/awaitable.hpp>

#include <iostream>

namespace asio = boost::asio;
namespace ch   = std::chrono;

asio::awaitable<void> run_exmaple(asiofiedpq::connection& conn)
{
  auto r1 = co_await conn.async_query("SELECT $1 as a, $2 as b, $3 as c;", { "one", 2, 3.5 }, asio::deferred);

  const auto [a, b, c] = r1.at(0).as<std::string_view, int, double>();
  std::cout << a << "-" << b << "-" << c << std::endl;

  auto r2 = co_await conn.async_query("SELECT $1 as array;", std::vector{ "1", "2", "3" }, asio::deferred);
  for (const auto& value : r2.at(0).at("array").as<std::vector<std::string_view>>())
    std::cout << value << std::endl;

  auto r3       = co_await conn.async_query("SELECT 1 as f1, (2.0::FLOAT4, '3'::TEXT) as f2;", asio::deferred);
  auto [f1, f2] = r3.at(0).as<int, std::tuple<float, std::string_view>>();
  std::cout << f1 << std::get<0>(f2) << std::get<1>(f2) << std::endl;

  auto r4      = co_await conn.async_query("SELECT NOW()::timestamp(0) as timestamp;", asio::deferred);
  auto db_time = r4.at(0).at("timestamp").as<ch::system_clock::time_point>().time_since_epoch();
  std::cout << "database timestamp:" << ch::duration_cast<ch::seconds>(db_time) << std::endl;
}
