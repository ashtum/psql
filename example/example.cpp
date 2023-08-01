#include <asiofiedpq/connection.hpp>

#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/io_context.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> phonebook_demo(asiofiedpq::connection& conn)
{
  auto pipeline = std::vector<asiofiedpq::pipelined>{};

  pipeline.push_back({ "DROP TABLE IF EXISTS phonebook;" });
  pipeline.push_back({ "CREATE TABLE phonebook(phone VARCHAR, name VARCHAR);" });
  pipeline.push_back({ "INSERT INTO phonebook VALUES ($1, $2);", { "+1 111 444 7777", "Jake" } });
  pipeline.push_back({ "INSERT INTO phonebook VALUES ($1, $2);", { "+2 333 222 3333", "Megan" } });
  pipeline.push_back({ "SELECT * FROM phonebook ORDER BY name;" });

  co_await conn.async_exec_pipeline(pipeline.begin(), pipeline.end(), asio::deferred);

  PGresult* phonebook = pipeline.back().result.get();
  for (auto i = 0; i < PQntuples(phonebook); i++)
  {
    for (auto j = 0; j < PQnfields(phonebook); j++)
      std::cout << PQfname(phonebook, j) << ":" << PQgetvalue(phonebook, i, j) << '\t';
    std::cout << std::endl;
  }

  auto print = [](auto res) { std::cout << PQfname(res.get(), 0) << ':' << PQgetvalue(res.get(), 0, 0) << std::endl; };

  print(co_await conn.async_query("SELECT $1 as int_array;", std::vector{ 1, 2, 3 }, asio::deferred));

  print(co_await conn.async_query("SELECT $1 as bool_array;", std::vector{ true, false, true }, asio::deferred));

  print(co_await conn.async_query("SELECT $1 as timepoint;", std::chrono::system_clock::now(), asio::deferred));

  print(co_await conn.async_query("SELECT NOW()::timestamp(0) as timestamp;", asio::deferred));
}

asio::awaitable<void> async_main()
{
  using namespace asio::experimental::awaitable_operators;

  auto conn = asiofiedpq::connection{ co_await asio::this_coro::executor };

  co_await conn.async_connect("postgresql://postgres:postgres@172.18.0.2:5432", asio::deferred);

  co_await (conn.async_run(asio::use_awaitable) || phonebook_demo(conn));
}

int main()
{
  try
  {
    auto ioc = asio::io_context{};

    asio::co_spawn(
      ioc,
      async_main(),
      [](const std::exception_ptr& ep)
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
