#pragma once

#include <cinttypes>

namespace psql
{
namespace detail
{
struct oid_pair
{
  uint32_t single = {};
  uint32_t array  = {};
};
} // namespace detail
} // namespace psql
