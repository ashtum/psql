#pragma once

#include <chrono>
#include <cstdint>

namespace asiofiedpq
{
template<typename>
struct oid_map;

template<>
struct oid_map<bool>
{
  static constexpr auto value     = 16;
  static constexpr auto arr_value = 1000;
};

template<>
struct oid_map<std::byte>
{
  static constexpr auto value     = 17;
  static constexpr auto arr_value = 1001;
};

template<>
struct oid_map<char>
{
  static constexpr auto value     = 18;
  static constexpr auto arr_value = 1002;
};

template<>
struct oid_map<int64_t>
{
  static constexpr auto value     = 20;
  static constexpr auto arr_value = 1016;
};

template<>
struct oid_map<int16_t>
{
  static constexpr auto value     = 21;
  static constexpr auto arr_value = 1005;
};

template<>
struct oid_map<int32_t>
{
  static constexpr auto value     = 23;
  static constexpr auto arr_value = 1007;
};

template<>
struct oid_map<float>
{
  static constexpr auto value     = 700;
  static constexpr auto arr_value = 1021;
};

template<>
struct oid_map<double>
{
  static constexpr auto value     = 701;
  static constexpr auto arr_value = 1022;
};

template<>
struct oid_map<std::chrono::system_clock::time_point>
{
  static constexpr auto value     = 1114;
  static constexpr auto arr_value = 1115;
};

} // namespace asiofiedpq
