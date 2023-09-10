#pragma once

#include <psql/detail/oid_pair.hpp>

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace psql
{
namespace detail
{
template<typename>
struct builtin;

template<>
struct builtin<uint32_t>
{
  static constexpr oid_pair oids{ 26, 1028 };
};

template<>
struct builtin<bool>
{
  static constexpr oid_pair oids{ 16, 1000 };
};

template<>
struct builtin<std::byte>
{
  static constexpr oid_pair oids{ 17, 1001 };
};

template<>
struct builtin<char>
{
  static constexpr oid_pair oids{ 18, 1002 };
};

template<>
struct builtin<int64_t>
{
  static constexpr oid_pair oids{ 20, 1016 };
};

template<>
struct builtin<int16_t>
{
  static constexpr oid_pair oids{ 21, 1005 };
};

template<>
struct builtin<int32_t>
{
  static constexpr oid_pair oids{ 23, 1007 };
};

template<>
struct builtin<float>
{
  static constexpr oid_pair oids{ 700, 1021 };
};

template<>
struct builtin<double>
{
  static constexpr oid_pair oids{ 701, 1022 };
};

template<>
struct builtin<std::chrono::system_clock::time_point>
{
  static constexpr oid_pair oids{ 1114, 1115 };
};

template<>
struct builtin<std::string_view>
{
  static constexpr oid_pair oids{ 25, 1009 };
};

template<>
struct builtin<std::string>
{
  static constexpr oid_pair oids{ 25, 1009 };
};

template<>
struct builtin<const char*>
{
  static constexpr oid_pair oids{ 25, 1009 };
};

template<typename... Ts>
struct builtin<std::tuple<Ts...>>
{
  static constexpr oid_pair oids{ 2249, 2287 };
};
} // namespace detail
} // namespace psql
