#pragma once

#include <psql/detail/oid_pair.hpp>

#include <map>
#include <typeindex>

namespace psql
{
namespace detail
{
using oid_map = std::map<std::type_index, oid_pair>;
} // namespace detail
} // namespace psql
