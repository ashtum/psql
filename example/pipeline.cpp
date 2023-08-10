#include <asiofiedpq/connection.hpp>

#include <boost/asio/awaitable.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> run_exmaple(asiofiedpq::connection& conn)
{
  auto pipeline = std::vector<asiofiedpq::pipelined>{};

  pipeline.push_back({ "DROP TABLE IF EXISTS phonebook;" });
  pipeline.push_back({ "CREATE TABLE phonebook(phone VARCHAR, name VARCHAR);" });
  pipeline.push_back({ "INSERT INTO phonebook VALUES ($1, $2);", { "+1 111 444 7777", "Jake" } });
  pipeline.push_back({ "INSERT INTO phonebook VALUES ($1, $2);", { "+2 333 222 3333", "Megan" } });
  pipeline.push_back({ "SELECT * FROM phonebook ORDER BY name;" });

  co_await conn.async_exec_pipeline(pipeline.begin(), pipeline.end(), asio::deferred);

  for (auto row : pipeline.back().result)
  {
    for (auto value : row)
      std::cout << value.name() << ":" << value.data() << '\t';

    std::cout << std::endl;
  }
}
