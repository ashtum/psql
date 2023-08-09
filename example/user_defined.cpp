#include <asiofiedpq/connection.hpp>
#include <asiofiedpq/query_oids.hpp>

#include <boost/asio/awaitable.hpp>

#include <iostream>

namespace asio = boost::asio;

struct Employee
{
  std::string name;
  std::string phone;
};

struct Company
{
  int64_t id;
  std::vector<Employee> employees;
};

namespace asiofiedpq
{
template<>
struct user_defined<Employee>
{
  static constexpr auto members = std::tuple{ &Employee::name, &Employee::phone };
};

template<>
struct user_defined<Company>
{
  static constexpr auto members = std::tuple{ &Company::id, &Company::employees };
};
}

asio::awaitable<void> run_exmaple(asiofiedpq::connection& conn)
{
  co_await conn.async_query("DROP TYPE IF EXISTS employee;", asio::deferred);
  co_await conn.async_query("DROP TYPE IF EXISTS company;", asio::deferred);
  co_await conn.async_query("CREATE TYPE employee AS (name TEXT, phone TEXT);", asio::deferred);
  co_await conn.async_query("CREATE TYPE company AS (id INT8, employees employee[]);", asio::deferred);

  auto oid_map = asiofiedpq::oid_map{};
  oid_map.register_type<Employee>("employee");
  oid_map.register_type<Company>("company");

  co_await asiofiedpq::async_query_oids(conn, oid_map, asio::deferred);

  auto company = Company{ 104, { { "John Doe", "555-123-4567" }, { "Jane Smith", "555-987-6543" } } };

  auto result = co_await conn.async_query("SELECT $1::company;", { oid_map, company }, asio::deferred);

  std::cout << PQgetvalue(result.get(), 0, 0) << std::endl;
}
