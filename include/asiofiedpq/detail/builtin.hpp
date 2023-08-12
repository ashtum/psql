#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace asiofiedpq
{
namespace detail
{
template<typename>
struct builtin;

template<>
struct builtin<uint32_t>
{
  static constexpr auto type_oid  = 26;
  static constexpr auto array_oid = 1028;
};

template<>
struct builtin<bool>
{
  static constexpr auto type_oid  = 16;
  static constexpr auto array_oid = 1000;
};

template<>
struct builtin<std::byte>
{
  static constexpr auto type_oid  = 17;
  static constexpr auto array_oid = 1001;
};

template<>
struct builtin<char>
{
  static constexpr auto type_oid  = 18;
  static constexpr auto array_oid = 1002;
};

template<>
struct builtin<int64_t>
{
  static constexpr auto type_oid  = 20;
  static constexpr auto array_oid = 1016;
};

template<>
struct builtin<int16_t>
{
  static constexpr auto type_oid  = 21;
  static constexpr auto array_oid = 1005;
};

template<>
struct builtin<int32_t>
{
  static constexpr auto type_oid  = 23;
  static constexpr auto array_oid = 1007;
};

template<>
struct builtin<float>
{
  static constexpr auto type_oid  = 700;
  static constexpr auto array_oid = 1021;
};

template<>
struct builtin<double>
{
  static constexpr auto type_oid  = 701;
  static constexpr auto array_oid = 1022;
};

template<>
struct builtin<std::chrono::system_clock::time_point>
{
  static constexpr auto type_oid  = 1114;
  static constexpr auto array_oid = 1115;
};

template<>
struct builtin<std::string_view>
{
  static constexpr auto type_oid  = 25;
  static constexpr auto array_oid = 1009;
};

template<>
struct builtin<std::string>
{
  static constexpr auto type_oid  = 25;
  static constexpr auto array_oid = 1009;
};

template<>
struct builtin<const char*>
{
  static constexpr auto type_oid  = 25;
  static constexpr auto array_oid = 1009;
};

template<typename... Ts>
struct builtin<std::tuple<Ts...>>
{
  static constexpr auto type_oid  = 2249;
  static constexpr auto array_oid = 2287;
};
} // namespace detail
} // namespace asiofiedpq
