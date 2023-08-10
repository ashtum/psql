#pragma once

#include <asiofiedpq/detail/serialization.hpp>

#include <libpq-fe.h>

#include <array>
#include <memory>

namespace asiofiedpq
{
class params
{
  std::string buffer_;
  std::vector<Oid> types_;
  std::vector<const char*> values_;
  std::vector<int> lengths_;
  std::vector<int> formats_;

  static const inline oid_map empty_omp;

public:
  params() = default;

  template<typename... Ts>
  params(const oid_map& omp, Ts&&... args)
    requires(!(std::is_same_v<asiofiedpq::params, std::decay_t<Ts>> || ...))
  {
    (add(omp, std::forward<Ts>(args)), ...);
    convert_offsets();
  }

  template<typename... Ts>
  params(Ts&&... args)
    requires(
      !((std::is_same_v<oid_map, std::decay_t<Ts>> || std::is_same_v<asiofiedpq::params, std::decay_t<Ts>>) || ...))
    : params{ empty_omp, std::forward<Ts>(args)... }
  {
  }

  int count() const
  {
    return types_.size();
  }

  const Oid* types() const
  {
    return types_.data();
  }

  const char* const* values() const
  {
    return values_.data();
  }

  const int* lengths() const
  {
    return lengths_.data();
  }

  const int* formats() const
  {
    return formats_.data();
  }

private:
  template<typename T>
  void add(const oid_map& omp, const T& value)
  {
    types_.push_back(detail::oid_of<std::decay_t<T>>(omp));
    lengths_.push_back(detail::size_of(value));
    detail::serialize(omp, &buffer_, value);
  }

  void convert_offsets()
  {
    for (size_t offset = 0; const auto& length : lengths_)
    {
      values_.push_back(length ? buffer_.data() + offset : nullptr);
      offset += length;
      formats_.push_back(1); // All items are in binary format
    }
  }
};
} // namespace asiofiedpq
