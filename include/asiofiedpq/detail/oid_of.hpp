#pragma once

#include <asiofiedpq/detail/builtin.hpp>
#include <asiofiedpq/detail/type_traits.hpp>
#include <asiofiedpq/oid_map.hpp>

namespace asiofiedpq
{
namespace detail
{
template<class T>
struct oid_of_impl;

template<typename T>
int oid_of(const oid_map& omp)
{
  return oid_of_impl<std::decay_t<T>>::apply(omp);
}

template<typename T>
  requires is_array<T>::value
struct oid_of_impl<T>
{
  using value_type = typename T::value_type;

  static int apply(const oid_map&)
    requires(!is_user_defined<value_type>::value)
  {
    return builtin<value_type>::array_oid;
  }

  static int apply(const oid_map& omp)
    requires is_user_defined<value_type>::value
  {
    return omp.get_array_oid<value_type>();
  }
};

template<typename T>
  requires(!is_array<T>::value)
struct oid_of_impl<T>
{
  static int apply(const oid_map&)
    requires(!is_user_defined<T>::value)
  {
    return builtin<T>::type_oid;
  }

  static int apply(const oid_map& omp)
    requires is_user_defined<T>::value
  {
    return omp.get_type_oid<T>();
  }
};
} // namespace detail
} // namespace asiofiedpq
