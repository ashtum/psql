#pragma once

#include <tuple>
#include <vector>

namespace psql
{
template<typename>
struct user_defined;

template<typename>
struct is_user_defined : std::false_type
{
};

template<typename T>
  requires requires(T) { user_defined<T>{}; }
struct is_user_defined<T> : std::true_type
{
};

namespace detail
{
template<class T>
struct is_array : std::false_type
{
};

template<class T>
struct is_array<std::vector<T>> : std::true_type
{
};

template<typename>
struct is_tuple : std::false_type
{
};

template<typename... T>
struct is_tuple<std::tuple<T...>> : std::true_type
{
};

template<class T>
struct is_composite : std::false_type
{
};

template<class T>
  requires is_user_defined<T>::value
struct is_composite<T> : std::true_type
{
};

template<class T>
  requires is_tuple<T>::value
struct is_composite<T> : std::true_type
{
};
} // namespace detail
} // namespace psql
