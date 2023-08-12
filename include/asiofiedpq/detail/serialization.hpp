#pragma once

#include <asiofiedpq/detail/oid_of.hpp>
#include <asiofiedpq/detail/size_of.hpp>

#include <boost/endian.hpp>

namespace asiofiedpq
{
namespace detail
{
template<class T>
struct serialize_impl;

template<typename T>
void serialize(const oid_map& omp, std::string* buffer, const T& v)
{
  serialize_impl<std::decay_t<T>>::apply(omp, buffer, v);
}

template<typename T>
  requires(std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, std::byte>)
struct serialize_impl<T>
{
  static void apply(const oid_map&, std::string* buffer, const T& value)
  {
    buffer->resize(buffer->size() + sizeof(T));
    auto* p = reinterpret_cast<unsigned char*>(std::addressof(buffer->back()) - (sizeof(T) - 1));
    boost::endian::endian_store<T, sizeof(T), boost::endian::order::big>(p, value);
  }
};

template<>
struct serialize_impl<std::chrono::system_clock::time_point>
{
  static void apply(const oid_map& omp, std::string* buffer, const std::chrono::system_clock::time_point& value)
  {
    const int64_t int_value = (std::chrono::duration_cast<std::chrono::microseconds>(value.time_since_epoch()) -
                               std::chrono::microseconds{ 946684800000000 })
                                .count();
    serialize(omp, buffer, int_value);
  }
};

template<>
struct serialize_impl<const char*>
{
  static void apply(const oid_map&, std::string* buffer, const char* value)
  {
    buffer->append(value);
  }
};

template<>
struct serialize_impl<std::string_view>
{
  static void apply(const oid_map&, std::string* buffer, const std::string_view& value)
  {
    buffer->append(value);
  }
};

template<>
struct serialize_impl<std::string>
{
  static void apply(const oid_map&, std::string* buffer, const std::string& value)
  {
    buffer->append(value);
  }
};

template<typename T>
  requires is_composite<T>::value
struct serialize_impl<T>
{
  template<typename U>
  static void serialize_member(const oid_map& omp, std::string* buffer, const U& value)
  {
    serialize<int32_t>(omp, buffer, oid_of<U>(omp));
    serialize<int32_t>(omp, buffer, size_of(value));
    serialize(omp, buffer, value);
  }

  static void apply(const oid_map& omp, std::string* buffer, const T& value)
    requires is_user_defined<T>::value
  {
    serialize<int32_t>(omp, buffer, std::tuple_size_v<decltype(user_defined<T>::members)>);
    std::apply([&](auto&&... mems) { (serialize_member(omp, buffer, value.*mems), ...); }, user_defined<T>::members);
  }

  static void apply(const oid_map& omp, std::string* buffer, const T& value)
    requires is_tuple<T>::value
  {
    serialize<int32_t>(omp, buffer, std::tuple_size_v<T>);
    std::apply([&](auto&&... mems) { (serialize_member(omp, buffer, mems), ...); }, value);
  }
};

template<typename T>
  requires is_array<T>::value
struct serialize_impl<T>
{
  using value_type = std::decay_t<typename T::value_type>;

  static void apply(const oid_map& omp, std::string* buffer, const T& array)
  {
    serialize<int32_t>(omp, buffer, 1);
    serialize<int32_t>(omp, buffer, 0);
    serialize<int32_t>(omp, buffer, oid_of<value_type>(omp));
    serialize<int32_t>(omp, buffer, std::size(array));
    serialize<int32_t>(omp, buffer, 0);

    for (const auto& value : array)
    {
      serialize<int32_t>(omp, buffer, size_of(value));
      serialize(omp, buffer, value);
    }
  }
};
} // namespace detail
} // namespace asiofiedpq
