#pragma once

#include <psql/detail/type_traits.hpp>

#include <chrono>
#include <string>
#include <string_view>

namespace psql
{
namespace detail
{
template<class T>
struct size_of_impl;

template<typename T>
std::size_t size_of(const T& v)
{
  return size_of_impl<std::decay_t<T>>::apply(v);
}

template<typename T>
  requires(std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, std::byte>)
struct size_of_impl<T>
{
  static std::size_t apply(const T&)
  {
    return sizeof(T);
  }
};

template<>
struct size_of_impl<std::chrono::system_clock::time_point>
{
  static std::size_t apply(const std::chrono::system_clock::time_point&)
  {
    return 8;
  }
};

template<>
struct size_of_impl<const char*>
{
  static std::size_t apply(const char* value)
  {
    return std::string_view{ value }.size();
  }
};

template<>
struct size_of_impl<std::string_view>
{
  static std::size_t apply(const std::string_view& value)
  {
    return value.size();
  }
};

template<>
struct size_of_impl<std::string>
{
  static std::size_t apply(const std::string& value)
  {
    return value.size();
  }
};

template<typename T>
  requires is_array<T>::value
struct size_of_impl<T>
{
  static std::size_t apply(const T& vec)
  {
    std::size_t size = 20;
    for (const auto& value : vec)
      size += size_of(value) + 4;
    return size;
  }
};

template<typename T>
  requires is_composite<T>::value
struct size_of_impl<T>
{
  static std::size_t apply(const T& value)
    requires is_user_defined<T>::value
  {
    std::size_t size = 4;
    std::apply([&](auto&&... mems) { size += ((size_of(value.*mems) + 8) + ...); }, user_defined<T>::members);
    return size;
  }

  static std::size_t apply(const T& value)
    requires is_tuple<T>::value
  {
    std::size_t size = 4;
    std::apply([&](auto&&... mems) { size += ((size_of(mems) + 8) + ...); }, value);
    return size;
  }
};
} // namespace detail
} // namespace psql
