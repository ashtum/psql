#pragma once

#include <asiofiedpq/oid_map.hpp>

#include <boost/endian.hpp>

#include <libpq-fe.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace asiofiedpq
{
class params
{
  static constexpr auto BINARY_FORMAT = 1;

  std::vector<char> buffer_;
  std::vector<Oid> types_;
  std::vector<const char*> values_;
  std::vector<int> lengths_;
  std::vector<int> formats_;

public:
  params(auto&&... params)
    requires(!(std::is_same_v<asiofiedpq::params, std::decay_t<decltype(params)>> || ...))
  {
    (add(std::forward<decltype(params)>(params)), ...);

    // convert offsets to pointers
    for (auto& offset : values_)
      offset = std::next(buffer_.data(), reinterpret_cast<size_t>(offset));
  }

  params(const params&)            = delete;
  params& operator=(const params&) = delete;
  params(params&&)                 = default;
  params& operator=(params&&)      = default;

  int number_of_params() const
  {
    return static_cast<int>(types_.size());
  }

  const Oid* types() const
  {
    return std::addressof(types_.front());
  }

  const char* const* values() const
  {
    return std::addressof(values_.front());
  }

  const int* lengths() const
  {
    return std::addressof(lengths_.front());
  }

  const int* formats() const
  {
    return std::addressof(formats_.front());
    ;
  }

private:
  template<typename T>
  void add(T param)
    requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
  {
    store_current_offset();
    boost::endian::endian_store<T, sizeof(T), boost::endian::order::big>(
      reinterpret_cast<unsigned char*>(make_buffer(sizeof(T))), param);
    types_.push_back(oid_map<T>::value);
    lengths_.push_back(sizeof(T));
    formats_.push_back(BINARY_FORMAT);
  }

  void add(std::string_view param)
  {
    buffer_.reserve(buffer_.size() + param.size());
    store_current_offset();
    buffer_.insert(
      buffer_.end(),
      reinterpret_cast<const unsigned char*>(param.begin()),
      reinterpret_cast<const unsigned char*>(param.end()));
    types_.push_back(25); // Oid of text
    lengths_.push_back(param.size());
    formats_.push_back(BINARY_FORMAT);
  }

  char* make_buffer(size_t size)
  {
    buffer_.resize(buffer_.size() + size);
    return std::addressof(buffer_.back()) - size + 1;
  }

  void store_current_offset()
  {
    return values_.push_back(reinterpret_cast<const char*>(buffer_.size()));
  }
};
} // namespace asiofiedpq
