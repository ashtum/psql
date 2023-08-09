#include <asiofiedpq/connection.hpp>

#include <boost/asio/awaitable.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> run_exmaple(asiofiedpq::connection& conn)
{
  auto print = [](auto res)
  {
    for (auto i = 0; i < PQnfields(res.get()); i++)
      std::cout << PQfname(res.get(), i) << ":" << PQgetvalue(res.get(), 0, i) << '\t';

    std::cout << std::endl;
  };

  print(co_await conn.async_query("SELECT $1 as a, $2 as b, $3 as c;", { "one", 2, 3.0 }, asio::deferred));

  print(co_await conn.async_query("SELECT $1 as int_array;", std::vector{ 1, 2, 3 }, asio::deferred));

  print(co_await conn.async_query("SELECT $1 as bool_array;", std::vector{ true, false, true }, asio::deferred));

  print(co_await conn.async_query("SELECT NOW()::timestamp(0) as timestamp;", asio::deferred));
}
