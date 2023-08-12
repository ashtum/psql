#pragma once

#include <asiofiedpq/detail/oid_of.hpp>
#include <asiofiedpq/detail/size_of.hpp>

#include <boost/endian.hpp>

#include <span>

namespace asiofiedpq
{
namespace detail
{
template<class T>
struct deserialize_impl;

template<typename T>
void deserialize(const oid_map& omp, std::span<const char> buffer, T& v)
{
  deserialize_impl<std::decay_t<T>>::apply(omp, buffer, v);
}

template<typename T>
  requires(std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, std::byte>)
struct deserialize_impl<T>
{
  static void apply(const oid_map&, std::span<const char> buffer, T& value)
  {
    value = boost::endian::endian_load<T, sizeof(T), boost::endian::order::big>(
      reinterpret_cast<const unsigned char*>(buffer.data()));
  }
};

template<>
struct deserialize_impl<std::string_view>
{
  static void apply(const oid_map&, std::span<const char> buffer, std::string_view& value)
  {
    value = { buffer.begin(), buffer.end() };
  }
};

template<>
struct deserialize_impl<std::string>
{
  static void apply(const oid_map&, std::span<const char> buffer, std::string& value)
  {
    value.append(buffer.begin(), buffer.end());
  }
};

inline void deserialize_and_verify_oid(const oid_map& omp, std::span<const char> buffer, uint32_t expected_oid)
{
  uint32_t oid = {};
  deserialize<uint32_t>(omp, buffer, oid);

  if (expected_oid != oid)
    throw std::runtime_error{ "Mismatched Object Identifiers (OIDs) in received and expected types. Found " +
                              std::to_string(oid) + " instead of " + std::to_string(expected_oid) };
}

template<typename T>
  requires is_composite<T>::value
struct deserialize_impl<T>
{
  template<typename U>
  static void deserialize_member(const oid_map& omp, std::span<const char>& buffer, U& value)
  {
    deserialize_and_verify_oid(omp, buffer, oid_of<U>(omp));

    int32_t member_size = {};
    deserialize<int32_t>(omp, buffer.subspan(4), member_size);

    deserialize(omp, buffer.subspan(8, member_size), value);

    buffer = buffer.subspan(8 + member_size); // consumes buffer
  }

  static void verify_member_counts(const oid_map& omp, std::span<const char>& buffer, int32_t expected_count)
  {
    int32_t count = {};
    deserialize<int32_t>(omp, buffer, count);

    if (expected_count != count)
      throw std::runtime_error{ "Mismatched member counts in received and expected composite types. Found " +
                                std::to_string(count) + " instead of " + std::to_string(expected_count) };
  }

  static void apply(const oid_map& omp, std::span<const char> buffer, T& value)
    requires is_user_defined<T>::value
  {
    verify_member_counts(omp, buffer, std::tuple_size_v<decltype(user_defined<T>::members)>);
    buffer = buffer.subspan(4);
    std::apply([&](auto&&... mems) { (deserialize_member(omp, buffer, value.*mems), ...); }, user_defined<T>::members);
  }

  static void apply(const oid_map& omp, std::span<const char> buffer, T& value)
    requires is_tuple<T>::value
  {
    verify_member_counts(omp, buffer, std::tuple_size_v<T>);
    buffer = buffer.subspan(4);
    std::apply([&](auto&&... mems) { (deserialize_member(omp, buffer, mems), ...); }, value);
  }
};

template<typename T>
  requires is_array<T>::value
struct deserialize_impl<T>
{
  using value_type = std::decay_t<typename T::value_type>;

  static void apply(const oid_map& omp, std::span<const char> buffer, T& array)
  {
    int32_t dimensions_count = {};
    deserialize<int32_t>(omp, buffer.subspan(0), dimensions_count);

    if (dimensions_count != 1)
      throw std::runtime_error{ "Unexpected multidimensional array" };

    deserialize_and_verify_oid(omp, buffer.subspan(8), oid_of<value_type>(omp));

    int32_t size = {};
    deserialize<int32_t>(omp, buffer.subspan(12), size);

    buffer = buffer.subspan(20);
    array.resize(size);

    for (auto& value : array)
    {
      int32_t value_size = {};
      deserialize<int32_t>(omp, buffer, value_size);
      deserialize(omp, buffer.subspan(4, value_size), value);
      buffer = buffer.subspan(4 + value_size); // consumes buffer
    }
  }
};
} // namespace detail
} // namespace asiofiedpq
