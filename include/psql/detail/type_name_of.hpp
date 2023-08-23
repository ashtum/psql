#pragma once

#include <psql/detail/type_traits.hpp>

#include <string_view>

namespace psql
{
namespace detail
{
template<typename T>
struct type_name_of_impl
{
  static void apply(std::vector<std::string_view>&)
  {
  }
};

template<typename T>
void type_name_of(std::vector<std::string_view>& vec)
{
  return type_name_of_impl<std::decay_t<T>>::apply(vec);
}

template<typename T>
  requires is_array<T>::value
struct type_name_of_impl<T>
{
  static void apply(std::vector<std::string_view>& vec)
  {
    type_name_of<typename T::value_type>(vec);
  }
};

template<typename T>
struct type_tag
{
};

template<typename T>
  requires is_composite<T>::value
struct type_name_of_impl<T>
{
  static void apply(std::vector<std::string_view>& vec)
    requires is_user_defined<T>::value
  {
    vec.push_back(user_defined<T>::name);
    std::apply([&](auto&&... mems) { (type_name_of<decltype(T{}.*mems)>(vec), ...); }, user_defined<T>::members);
  }

  static void apply(std::vector<std::string_view>& vec)
    requires is_tuple<T>::value
  {
    [&]<typename... Ts>(type_tag<std::tuple<Ts...>>) { (type_name_of<Ts>(vec), ...); }(type_tag<T>{});
  }
};

} // namespace detail
} // namespace psql
