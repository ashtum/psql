#pragma once

#include <array>
#include <tuple>
#include <vector>

namespace psql
{
template<typename>
struct user_defined;

namespace detail
{
template<typename>
struct is_user_defined : std::false_type
{
};

template<typename T>
  requires(requires(T) { user_defined<T>{}; })
struct is_user_defined<T> : std::true_type
{
};

template<typename T>
constexpr bool is_user_defined_v = is_user_defined<T>::value;

template<typename T>
struct is_array : std::false_type
{
};

template<typename T>
struct is_array<std::vector<T>> : std::true_type
{
};

template<typename T, std::size_t N>
struct is_array<std::array<T, N>> : std::true_type
{
};

template<typename T>
constexpr bool is_array_v = is_array<T>::value;

template<typename>
struct is_tuple : std::false_type
{
};

template<typename... T>
struct is_tuple<std::tuple<T...>> : std::true_type
{
};

template<typename T>
constexpr bool is_tuple_v = is_tuple<T>::value;

template<typename T>
struct is_composite : std::false_type
{
};

template<typename T>
  requires(is_user_defined_v<T>)
struct is_composite<T> : std::true_type
{
};

template<typename T>
  requires(is_tuple_v<T>)
struct is_composite<T> : std::true_type
{
};

template<typename T>
constexpr bool is_composite_v = is_composite<T>::value;
} // namespace detail
} // namespace psql
