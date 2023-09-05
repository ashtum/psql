#pragma once

#include <psql/detail/oid_map.hpp>
#include <psql/detail/type_traits.hpp>

#include <string_view>

namespace psql
{
namespace detail
{
template<typename T>
struct type_name_of_impl
{
  static constexpr void apply(std::vector<std::string_view>&, const detail::oid_map&)
  {
  }
};

template<typename T>
void constexpr type_name_of(std::vector<std::string_view>& vec, const detail::oid_map& omp)
{
  return type_name_of_impl<std::decay_t<T>>::apply(vec, omp);
}

template<typename... Params>
void constexpr extract_user_defined_types_names(std::vector<std::string_view>& vec, const detail::oid_map& omp)
{
  (type_name_of<Params>(vec, omp), ...);
}

template<typename T>
  requires(is_array_v<T>)
struct type_name_of_impl<T>
{
  static constexpr void apply(std::vector<std::string_view>& vec, const detail::oid_map& omp)
  {
    type_name_of<typename T::value_type>(vec, omp);
  }
};

template<typename T>
  requires(is_composite_v<T>)
struct type_name_of_impl<T>
{
  template<typename U>
  struct type_tag
  {
  };

  static constexpr void apply(std::vector<std::string_view>& vec, const detail::oid_map& omp)
    requires(is_user_defined_v<T>)
  {
    if (!omp.contains(user_defined<T>::name))
      vec.push_back(user_defined<T>::name);

    std::apply([&](auto&&... ms) { (type_name_of<decltype(T{}.*ms)>(vec, omp), ...); }, user_defined<T>::members);
  }

  static constexpr void apply(std::vector<std::string_view>& vec, const detail::oid_map& omp)
    requires(is_tuple_v<T>)
  {
    [&]<typename... Ts>(type_tag<std::tuple<Ts...>>) { (type_name_of<Ts>(vec, omp), ...); }(type_tag<T>{});
  }
};

} // namespace detail
} // namespace psql
