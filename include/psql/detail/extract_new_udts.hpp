#pragma once

#include <psql/detail/oid_map.hpp>
#include <psql/detail/type_traits.hpp>
#include <psql/detail/udt_pair.hpp>

namespace psql
{
namespace detail
{
template<typename T>
struct extract_new_udts_impl
{
  static constexpr void apply(std::vector<udt_pair>&, const detail::oid_map&)
  {
  }
};

template<typename T>
void constexpr extract_new_udts(std::vector<udt_pair>& new_udts, const detail::oid_map& omp)
{
  return extract_new_udts_impl<std::decay_t<T>>::apply(new_udts, omp);
}

template<typename T>
  requires(is_array_v<T>)
struct extract_new_udts_impl<T>
{
  static constexpr void apply(std::vector<udt_pair>& new_udts, const detail::oid_map& omp)
  {
    extract_new_udts<typename T::value_type>(new_udts, omp);
  }
};

template<typename T>
  requires(is_composite_v<T>)
struct extract_new_udts_impl<T>
{
  template<typename U>
  struct type_tag
  {
  };

  static constexpr void apply(std::vector<udt_pair>& new_udts, const detail::oid_map& omp)
    requires(is_user_defined_v<T>)
  {
    if (!omp.contains(typeid(T)))
      new_udts.push_back({ user_defined<T>::name, typeid(T) });

    std::apply(
      [&](auto&&... ms) { (extract_new_udts<decltype(T{}.*ms)>(new_udts, omp), ...); }, user_defined<T>::members);
  }

  static constexpr void apply(std::vector<udt_pair>& new_udts, const detail::oid_map& omp)
    requires(is_tuple_v<T>)
  {
    [&]<typename... Ts>(type_tag<std::tuple<Ts...>>) { (extract_new_udts<Ts>(new_udts, omp), ...); }(type_tag<T>{});
  }
};
} // namespace detail
} // namespace psql
