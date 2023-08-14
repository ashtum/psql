#pragma once

#include <psql/detail/builtin.hpp>
#include <psql/detail/type_traits.hpp>
#include <psql/oid_map.hpp>

namespace psql
{
namespace detail
{
template<class T>
struct oid_of_impl;

template<typename T>
uint32_t oid_of(const oid_map& omp)
{
  return oid_of_impl<std::decay_t<T>>::apply(omp);
}

template<typename T>
  requires is_array<T>::value
struct oid_of_impl<T>
{
  using value_type = std::decay_t<typename T::value_type>;

  static uint32_t apply(const oid_map&)
    requires(!is_user_defined<value_type>::value)
  {
    return builtin<value_type>::array_oid;
  }

  static uint32_t apply(const oid_map& omp)
    requires is_user_defined<value_type>::value
  {
    return omp.get_array_oid<value_type>();
  }
};

template<typename T>
  requires(!is_array<T>::value)
struct oid_of_impl<T>
{
  static uint32_t apply(const oid_map&)
    requires(!is_user_defined<T>::value)
  {
    return builtin<T>::type_oid;
  }

  static uint32_t apply(const oid_map& omp)
    requires is_user_defined<T>::value
  {
    return omp.get_type_oid<T>();
  }
};
} // namespace detail
} // namespace psql
