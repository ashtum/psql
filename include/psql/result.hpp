#pragma once

#include <psql/detail/deserialization.hpp>

#include <libpq-fe.h>

#include <memory>

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

class row
{
  const PGresult* pg_result_{};
  int row_{};

public:
  class const_iterator;

  row() = default;

  row(const PGresult* pg_result, int row)
    : pg_result_{ pg_result }
    , row_{ row }
  {
  }

  const_iterator begin() const noexcept;

  const_iterator end() const noexcept;

  const row* operator->() const noexcept
  {
    return this;
  }

  cell operator[](int index) const noexcept
  {
    return cell{ pg_result_, row_, index };
  }

  cell at(int index) const
  {
    if (static_cast<size_t>(index) < size())
      return cell{ pg_result_, row_, index };

    throw std::out_of_range{ std::string{ "No field at index " } + std::to_string(index) + " exists" };
  }

  cell at(const char* name) const
  {
    if (auto i = PQfnumber(pg_result_, name); i != -1)
      return cell{ pg_result_, row_, i };

    throw std::out_of_range{ std::string{ "No field named " } + name + " exists" };
  }

  size_t size() const noexcept
  {
    return PQnfields(pg_result_);
  }

  [[nodiscard]] bool empty() const noexcept
  {
    return size() == 0;
  }
};

template<typename T>
auto as(const row& row, const oid_map& omp = empty_omp)
{
  return as<T>(row.at(0), omp);
}

template<typename T1, typename T2, typename... Ts>
auto as(const row& row, const oid_map& omp = empty_omp)
{
  return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
    return std::tuple{ as<T1>(row.at(0), omp), as<T2>(row.at(1), omp), as<Ts>(row.at(Is + 2), omp)... };
  }(std::index_sequence_for<Ts...>{});
}

class row::const_iterator
{
  const PGresult* pg_result_{};
  int row_{};
  int col_{};

public:
  using value_type        = cell;
  using difference_type   = std::ptrdiff_t;
  using iterator_category = std::bidirectional_iterator_tag;
  using pointer           = cell;
  using reference         = cell;

  const_iterator() = default;

  const_iterator(const PGresult* pg_result, int row, int col)
    : pg_result_{ pg_result }
    , row_{ row }
    , col_{ col }
  {
  }

  const_iterator operator++(int)
  {
    const auto tmp = *this;
    ++*this;
    return tmp;
  }

  const_iterator& operator++()
  {
    col_++;
    return *this;
  }

  const_iterator operator--(int)
  {
    const auto tmp = *this;
    --*this;
    return tmp;
  }

  const_iterator& operator--()
  {
    col_--;
    return *this;
  }

  bool operator!=(const const_iterator& rhs) const
  {
    return !(*this == rhs);
  }

  bool operator==(const const_iterator& rhs) const
  {
    return pg_result_ == rhs.pg_result_ && row_ == rhs.row_ && col_ == rhs.col_;
  }

  cell operator*() const
  {
    return cell{ pg_result_, row_, col_ };
  }

  cell operator->() const
  {
    return cell{ pg_result_, row_, col_ };
  }
};

inline row::const_iterator row::begin() const noexcept
{
  return const_iterator{ pg_result_, row_, 0 };
}

inline row::const_iterator row::end() const noexcept
{
  return const_iterator{ pg_result_, row_, static_cast<int>(size()) };
}

class result
{
  struct pgresult_deleter
  {
    void operator()(PGresult* p)
    {
      PQclear(p);
    }
  };

  std::unique_ptr<PGresult, pgresult_deleter> pg_result_;

public:
  class const_iterator;

  result() = default;

  result(PGresult* pg_result)
    : pg_result_{ pg_result }
  {
  }

  const_iterator begin() const noexcept;

  const_iterator end() const noexcept;

  row operator[](int index) const noexcept
  {
    return row{ pg_result_.get(), index };
  }

  row at(int index) const
  {
    if (static_cast<size_t>(index) < size())
      return row{ pg_result_.get(), index };

    throw std::out_of_range{ std::string{ "No row at index " } + std::to_string(index) + " exists" };
  }

  operator bool() const noexcept
  {
    return !!pg_result_;
  }

  size_t size() const noexcept
  {
    return PQntuples(pg_result_.get());
  }

  [[nodiscard]] bool empty() const noexcept
  {
    return size() == 0;
  }

  PGresult* native_handle() const noexcept
  {
    return pg_result_.get();
  }

  std::string_view error_message() const noexcept
  {
    return PQresultErrorMessage(pg_result_.get());
  }

  [[nodiscard]] PGresult* release() noexcept
  {
    return pg_result_.release();
  }
};

class result::const_iterator
{
  const PGresult* pg_result_{};
  int row_{};

public:
  using value_type        = row;
  using difference_type   = std::ptrdiff_t;
  using iterator_category = std::bidirectional_iterator_tag;
  using pointer           = row;
  using reference         = row;

  const_iterator() = default;

  const_iterator(const PGresult* pg_result, int row)
    : pg_result_{ pg_result }
    , row_{ row }
  {
  }

  const_iterator operator++(int)
  {
    const auto tmp = *this;
    ++*this;
    return tmp;
  }

  const_iterator& operator++()
  {
    row_++;
    return *this;
  }

  const_iterator operator--(int)
  {
    const auto tmp = *this;
    --*this;
    return tmp;
  }

  const_iterator& operator--()
  {
    row_--;
    return *this;
  }

  bool operator!=(const const_iterator& rhs) const
  {
    return !(*this == rhs);
  }

  bool operator==(const const_iterator& rhs) const
  {
    return pg_result_ == rhs.pg_result_ && row_ == rhs.row_;
  }

  row operator*() const
  {
    return row{ pg_result_, row_ };
  }

  row operator->() const
  {
    return row{ pg_result_, row_ };
  }
};

inline result::const_iterator result::begin() const noexcept
{
  return const_iterator{ pg_result_.get(), 0 };
}

inline result::const_iterator result::end() const noexcept
{
  return const_iterator{ pg_result_.get(), static_cast<int>(size()) };
}

template<typename... Ts>
auto as(const result& result, const oid_map& omp = empty_omp)
{
  return as<Ts...>(result.at(0), omp);
}
} // namespace psql
