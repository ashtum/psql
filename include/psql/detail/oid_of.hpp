#pragma once

#include <psql/detail/builtins.hpp>
#include <psql/detail/oid_map.hpp>
#include <psql/detail/type_traits.hpp>

namespace psql
{
namespace detail
{
template<class T>
struct oid_of_impl;

template<typename T>
constexpr uint32_t oid_of(const oid_map& omp)
{
  return oid_of_impl<std::decay_t<T>>::apply(omp);
}

template<typename T>
constexpr uint32_t oid_of()
{
  return oid_of_impl<std::decay_t<T>>::apply();
}

template<typename T>
  requires(is_array_v<T>)
struct oid_of_impl<T>
{
  using value_type = std::decay_t<typename T::value_type>;

  static constexpr uint32_t apply(const oid_map&)
    requires(!is_user_defined_v<value_type>)
  {
    return builtin<value_type>::oids.array;
  }

  static constexpr uint32_t apply()
    requires(!is_user_defined_v<value_type>)
  {
    return builtin<value_type>::oids.array;
  }

  static constexpr uint32_t apply(const oid_map& omp)
    requires(is_user_defined_v<value_type>)
  {
    return omp.at(typeid(value_type)).array;
  }

  static constexpr uint32_t apply()
    requires(is_user_defined_v<value_type>)
  {
    return 0;
  }
};

template<typename T>
  requires(!is_array_v<T>)
struct oid_of_impl<T>
{
  static constexpr uint32_t apply(const oid_map&)
    requires(!is_user_defined_v<T>)
  {
    return builtin<T>::oids.single;
  }

  static constexpr uint32_t apply()
    requires(!is_user_defined_v<T>)
  {
    return builtin<T>::oids.single;
  }

  static constexpr uint32_t apply(const oid_map& omp)
    requires(is_user_defined_v<T>)
  {
    return omp.at(typeid(T)).single;
  }

  static constexpr uint32_t apply()
    requires(is_user_defined_v<T>)
  {
    return 0;
  }
};
} // namespace detail
} // namespace psql
