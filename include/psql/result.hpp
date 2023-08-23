#pragma once

#include <psql/row.hpp>

#include <memory>

namespace psql
{
class result
{
  struct pgresult_deleter
  {
    void operator()(PGresult* p)
    {
      PQclear(p);
    }
  };

  std::unique_ptr<PGresult, pgresult_deleter> pgresult_;

public:
  class const_iterator;

  result() = default;

  explicit result(PGresult* pg_result)
    : pgresult_{ pg_result }
  {
  }

  const_iterator begin() const noexcept;

  const_iterator end() const noexcept;

  row operator[](int index) const noexcept
  {
    return row{ pgresult_.get(), index };
  }

  row at(int index) const
  {
    if (static_cast<size_t>(index) < size())
      return row{ pgresult_.get(), index };

    throw std::out_of_range{ std::string{ "No row at index " } + std::to_string(index) + " exists" };
  }

  operator bool() const noexcept
  {
    return !!pgresult_;
  }

  size_t size() const noexcept
  {
    return PQntuples(pgresult_.get());
  }

  [[nodiscard]] bool empty() const noexcept
  {
    return size() == 0;
  }

  PGresult* native_handle() const noexcept
  {
    return pgresult_.get();
  }

  std::string_view error_message() const noexcept
  {
    return PQresultErrorMessage(pgresult_.get());
  }

  [[nodiscard]] PGresult* release() noexcept
  {
    return pgresult_.release();
  }
};

class result::const_iterator
{
  const PGresult* pgresult_{};
  int row_{};

public:
  using value_type        = row;
  using difference_type   = std::ptrdiff_t;
  using iterator_category = std::bidirectional_iterator_tag;
  using pointer           = row;
  using reference         = row;

  const_iterator() = default;

  const_iterator(const PGresult* pg_result, int row)
    : pgresult_{ pg_result }
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
    return pgresult_ == rhs.pgresult_ && row_ == rhs.row_;
  }

  row operator*() const
  {
    return row{ pgresult_, row_ };
  }

  row operator->() const
  {
    return row{ pgresult_, row_ };
  }
};

inline result::const_iterator result::begin() const noexcept
{
  return const_iterator{ pgresult_.get(), 0 };
}

inline result::const_iterator result::end() const noexcept
{
  return const_iterator{ pgresult_.get(), static_cast<int>(size()) };
}

template<typename... Ts>
auto as(const result& result)
{
  return as<Ts...>(result.at(0));
}
} // namespace psql
