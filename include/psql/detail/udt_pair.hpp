#pragma once

#include <typeindex>
#include <string_view>

namespace psql
{
namespace detail
{
struct udt_pair
{
  std::string_view name;
  std::type_index type_index;
};
} // namespace detail
} // namespace psql
