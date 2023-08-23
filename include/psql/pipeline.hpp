#pragma once

#include <psql/error.hpp>
#include <psql/params.hpp>

namespace psql
{
struct pipeline
{
public:
  struct item
  {
    bool is_prepared;
    std::string query_or_stmt_name;
    psql::params params;
  };

  std::vector<item> items;

  void push_query(std::string query, params params = {})
  {
    items.push_back({ false, std::move(query), std::move(params) });
  }

  void push_query_prepared(std::string stmt_name, params params = {})
  {
    items.push_back({ true, std::move(stmt_name), std::move(params) });
  }
};
} // namespace psql
