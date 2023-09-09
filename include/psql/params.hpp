#pragma once

#include <tuple>

namespace psql
{
template<typename... Ts>
struct params : std::tuple<Ts...>
{
  using std::tuple<Ts...>::tuple;
};

template<typename... Ts>
constexpr auto mp(Ts&&... ts)
{
  return params<std::decay_t<Ts>...>{ std::forward<Ts>(ts)... };
}
} // namespace psql
