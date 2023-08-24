#include <psql/connection.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/deferred.hpp>

#include <iostream>

namespace asio = boost::asio;

struct Employee
{
  std::string name;
  std::string phone;
};

struct Company
{
  std::int64_t id;
  std::vector<Employee> employees;
};

namespace psql
{
template<>
struct user_defined<Employee>
{
  static constexpr auto name    = "employee";
  static constexpr auto members = std::tuple{ &Employee::name, &Employee::phone };
};

template<>
struct user_defined<Company>
{
  static constexpr auto name    = "company";
  static constexpr auto members = std::tuple{ &Company::id, &Company::employees };
};
}

asio::awaitable<void> run_exmaple(psql::connection& conn)
{
  co_await conn.async_query("DROP TYPE IF EXISTS company;", asio::deferred);
  co_await conn.async_query("DROP TYPE IF EXISTS employee;", asio::deferred);
  co_await conn.async_query("CREATE TYPE employee AS (name TEXT, phone TEXT);", asio::deferred);
  co_await conn.async_query("CREATE TYPE company AS (id INT8, employees employee[]);", asio::deferred);

  auto company = Company{ 104, { { "John Doe", "555-123-4567" }, { "Jane Smith", "555-987-6543" } } };

  auto result = co_await conn.async_query("SELECT $1;", std::tuple{ company }, asio::deferred);

  auto [id, employees] = as<Company>(result);

  std::cout << "company id:" << id << std::endl;

  std::cout << "company employees:" << std::endl;
  for (const auto& e : employees)
    std::cout << "name:" << e.name << '\t' << "phone:" << e.phone << '\t' << std::endl;
}
