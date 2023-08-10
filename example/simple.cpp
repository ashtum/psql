#include <asiofiedpq/connection.hpp>

#include <boost/asio/awaitable.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> run_exmaple(asiofiedpq::connection& conn)
{
  auto print = [](auto result)
  {
    for (auto row : result)
    {
      for (auto value : row)
        std::cout << value.name() << ":" << value.data() << '\t';

      std::cout << std::endl;
    }
  };

  print(co_await conn.async_query("SELECT $1 as a, $2 as b, $3 as c;", { "one", 2, 3.0 }, asio::deferred));

  print(co_await conn.async_query("SELECT $1 as int_array;", std::vector{ 1, 2, 3 }, asio::deferred));

  print(co_await conn.async_query("SELECT $1 as bool_array;", std::vector{ true, false, true }, asio::deferred));

  print(co_await conn.async_query("SELECT NOW()::timestamp(0) as timestamp;", asio::deferred));
}
