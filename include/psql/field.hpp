#pragma once

#include <psql/detail/deserialization.hpp>

#include <libpq-fe.h>

namespace psql
{
class cell
{
  const PGresult* pg_result_{};
  int row_{};
  int col_{};

public:
  cell() = default;

  cell(const PGresult* pg_result, int row, int col)
    : pg_result_{ pg_result }
    , row_{ row }
    , col_{ col }
  {
  }

  const cell* operator->() const noexcept
  {
    return this;
  }

  Oid oid() const noexcept
  {
    return PQftype(pg_result_, col_);
  }

  std::string_view name() const noexcept
  {
    return PQfname(pg_result_, col_);
  }

  const char* data() const noexcept
  {
    return PQgetvalue(pg_result_, row_, col_);
  }

  size_t size() const noexcept
  {
    return PQgetlength(pg_result_, row_, col_);
  }

  bool is_null() const noexcept
  {
    return PQgetisnull(pg_result_, row_, col_);
  }
};

template<typename T>
T as(const cell& cell, const oid_map& omp = empty_omp)
{
  auto result = T{};

  const auto expected_oid = detail::oid_of<T>(omp);
  if (expected_oid != cell.oid())
    throw std::runtime_error{ "Mismatched Object Identifiers (OIDs) in received and expected types. Found " +
                              std::to_string(cell.oid()) + " instead of " + std::to_string(expected_oid) };

  detail::deserialize(omp, { cell.data(), cell.size() }, result);

  return result;
}
} // namespace psql
