#pragma once

#include <psql/params.hpp>

namespace psql
{
class connection;

class pipeline
{
protected:
  enum class type
  {
    query,
    prepare,
    query_prepared
  };

  struct operation
  {
    type type;
    std::string stmt_name;
    std::string query;
    params params;
  };

  std::vector<operation> operations;

  friend connection;

public:
  void enque_query(std::string query, params params = {})
  {
    operations.push_back({ type::query, {}, std::move(query), std::move(params) });
  }

  void enque_prepare(std::string stmt_name, std::string query)
  {
    operations.push_back({ type::prepare, std::move(stmt_name), std::move(query), {} });
  }

  void enque_query_prepared(std::string stmt_name, std::string query, params params = {})
  {
    operations.push_back({ type::query_prepared, std::move(stmt_name), std::move(query), std::move(params) });
  }
};
} // namespace psql
