#pragma once

#include <cstdint>

namespace asiofiedpq
{
template<typename>
struct oid_map;

template<>
struct oid_map<bool>
{
  static constexpr auto value = 16;
};

template<>
struct oid_map<char>
{
  static constexpr auto value = 18;
};

template<>
struct oid_map<int64_t>
{
  static constexpr auto value = 20;
};

template<>
struct oid_map<int16_t>
{
  static constexpr auto value = 21;
};

template<>
struct oid_map<int32_t>
{
  static constexpr auto value = 23;
};

template<>
struct oid_map<float>
{
  static constexpr auto value = 700;
};

template<>
struct oid_map<double>
{
  static constexpr auto value = 701;
};
} // namespace asiofiedpq
