#pragma once

#include <psql/field.hpp>

namespace psql
{
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

  field operator[](int index) const noexcept
  {
    return field{ pg_result_, row_, index };
  }

  field at(int index) const
  {
    if (static_cast<size_t>(index) < size())
      return field{ pg_result_, row_, index };

    throw std::out_of_range{ std::string{ "No field at index " } + std::to_string(index) + " exists" };
  }

  field at(const char* name) const
  {
    if (auto i = PQfnumber(pg_result_, name); i != -1)
      return field{ pg_result_, row_, i };

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
  using value_type        = field;
  using difference_type   = std::ptrdiff_t;
  using iterator_category = std::bidirectional_iterator_tag;
  using pointer           = field;
  using reference         = field;

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

  field operator*() const
  {
    return field{ pg_result_, row_, col_ };
  }

  field operator->() const
  {
    return field{ pg_result_, row_, col_ };
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
} // namespace psql
