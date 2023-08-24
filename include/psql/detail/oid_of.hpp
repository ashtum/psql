#pragma once

#include <psql/detail/builtins.hpp>
#include <psql/detail/oid_map.hpp>
#include <psql/detail/type_traits.hpp>

#include <libpq-fe.h>

namespace psql
{
namespace detail
{
template<class T>
struct oid_of_impl;

template<typename T>
Oid oid_of(const oid_map& omp)
{
  return oid_of_impl<std::decay_t<T>>::apply(omp);
}

template<typename T>
Oid oid_of()
{
  return oid_of_impl<std::decay_t<T>>::apply();
}

template<typename T>
  requires is_array<T>::value
struct oid_of_impl<T>
{
  using value_type = std::decay_t<typename T::value_type>;

  static Oid apply(const oid_map&)
    requires(!is_user_defined<value_type>::value)
  {
    return builtin<value_type>::array_oid;
  }

  static Oid apply()
    requires(!is_user_defined<value_type>::value)
  {
    return builtin<value_type>::array_oid;
  }

  static Oid apply(const oid_map& omp)
    requires is_user_defined<value_type>::value
  {
    return omp.at(user_defined<value_type>::name).array;
  }

  static Oid apply()
    requires is_user_defined<value_type>::value
  {
    return 0;
  }
};

template<typename T>
  requires(!is_array<T>::value)
struct oid_of_impl<T>
{
  static Oid apply(const oid_map&)
    requires(!is_user_defined<T>::value)
  {
    return builtin<T>::type_oid;
  }

  static Oid apply()
    requires(!is_user_defined<T>::value)
  {
    return builtin<T>::type_oid;
  }

  static Oid apply(const oid_map& omp)
    requires is_user_defined<T>::value
  {
    return omp.at(user_defined<T>::name).type;
  }

  static Oid apply()
    requires is_user_defined<T>::value
  {
    return 0;
  }
};
} // namespace detail
} // namespace psql
