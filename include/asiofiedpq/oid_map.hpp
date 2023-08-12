#pragma once

#include <asiofiedpq/detail/type_traits.hpp>

#include <map>
#include <stdexcept>
#include <string>
#include <typeindex>

namespace asiofiedpq
{
template<typename>
struct user_defined;

class oid_map
{
  struct pg_type_info
  {
    std::string name;
    int type_oid  = -1;
    int array_oid = -1;
  };

  std::map<std::type_index, pg_type_info> types_;

public:
  template<typename T>
    requires is_user_defined<T>::value
  void register_type(std::string name)
  {
    types_.emplace(typeid(T), pg_type_info{ std::move(name) });
  }

  void set_type_oids(std::string_view name, int type_oid, int array_oid)
  {
    auto& pg_type_info     = find_pg_type_info(name);
    pg_type_info.type_oid  = type_oid;
    pg_type_info.array_oid = array_oid;
  }

  std::vector<std::string_view> get_type_names() const
  {
    std::vector<std::string_view> result;
    result.reserve(types_.size());

    for (const auto& [_, value] : types_)
      result.push_back(value.name);

    return result;
  }

  template<typename T>
    requires is_user_defined<T>::value
  int get_type_oid() const
  {
    return find_pg_type_info(typeid(T)).type_oid;
  }

  template<typename T>
    requires is_user_defined<T>::value
  int get_array_oid() const
  {
    return find_pg_type_info(typeid(T)).array_oid;
  }

private:
  const pg_type_info& find_pg_type_info(const std::type_index& type_index) const
  {
    if (auto it = types_.find(type_index); it != types_.end())
      return it->second;

    throw std::runtime_error{ "The specified type does not exist in the oid_map" };
  }

  pg_type_info& find_pg_type_info(std::string_view name)
  {
    for (auto& [_, value] : types_)
      if (value.name == name)
        return value;

    throw std::runtime_error{ "The specified type does not exist in the oid_map" };
  }
};

static const inline oid_map empty_omp;
} // namespace asiofiedpq
