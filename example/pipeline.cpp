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

  // A pipeline is beneficial when we need to dispatch a series of queries and await their results.
  // For example, a batch of INSERTs or SELECTs.
  auto results = co_await conn.async_exec_pipeline(
    [](psql::pipeline& p)
    {
      p.push_query("DROP TABLE IF EXISTS phonebook;");
      p.push_query("CREATE TABLE phonebook(phone TEXT, name TEXT);");
      p.push_query("INSERT INTO phonebook VALUES ($1, $2);", psql::mp("+1 111 444 7777", "Jake"));
      p.push_query("INSERT INTO phonebook VALUES ($1, $2);", psql::mp("+2 333 222 3333", "Amie"));
      p.push_query("SELECT * FROM phonebook ORDER BY name;");
    },
    asio::deferred);

  // We use the last item in the vector, which represents the outcome of the SELECT query.
  for (const auto row : results.back())
  {
    const auto [phone, name] = as<std::string_view, std::string_view>(row);
    std::cout << name << ":" << phone << std::endl;
  }
}
