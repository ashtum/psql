#pragma once

#include <map>
#include <string>

namespace psql
{
namespace detail
{
struct oid_pair
{
  uint32_t type  = {};
  uint32_t array = {};
};

using oid_map = std::map<std::string, oid_pair>;
} // namespace detail
} // namespace psql
