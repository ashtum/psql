#include <psql/connection.hpp>

#include <boost/asio/awaitable.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> run_exmaple(psql::connection& conn)
{
  auto pipeline = psql::pipeline{};

  pipeline.enque_query("DROP TABLE IF EXISTS phonebook;");
  pipeline.enque_query("CREATE TABLE phonebook(phone TEXT, name TEXT);");
  pipeline.enque_query("INSERT INTO phonebook VALUES ($1, $2);", { "+1 111 444 7777", "Jake" });
  pipeline.enque_query("INSERT INTO phonebook VALUES ($1, $2);", { "+2 333 222 3333", "Megan" });
  pipeline.enque_query("SELECT * FROM phonebook ORDER BY name;");

  auto results = co_await conn.async_exec_pipeline(pipeline, asio::deferred);

  for (const auto row : results.back())
  {
    const auto [phone, name] = as<std::string_view, std::string_view>(row);
    std::cout << name << ":" << phone << std::endl;
  }
}
