#pragma once

#include <psql/detail/serialization.hpp>
#include <psql/detail/type_name_of.hpp>

#include <libpq-fe.h>

#include <array>
#include <memory>

namespace psql
{
class params_base
{
public:
  virtual void do_serialize(std::string&, std::vector<Oid>&, std::vector<int>&, const detail::oid_map&) const = 0;

  template<typename T>
  static void
  add(std::string& buffer, std::vector<Oid>& types, std::vector<int>& lengths, const detail::oid_map& omp, T&& value)
  {
    types.push_back(detail::oid_of<std::decay_t<T>>(omp));
    lengths.push_back(detail::size_of<std::decay_t<T>>(value));
    detail::serialize<std::decay_t<T>>(omp, &buffer, value);
  }

  void serialize(
    std::string& buffer,
    std::vector<Oid>& types,
    std::vector<int>& lengths,
    std::vector<const char*>& values,
    std::vector<int>& formats,
    const detail::oid_map& omp) const
  {
    buffer.clear();
    types.clear();
    lengths.clear();
    values.clear();
    formats.clear();

    do_serialize(buffer, types, lengths, omp);

    for (size_t offset = 0; const auto& length : lengths)
    {
      values.push_back(length ? buffer.data() + offset : nullptr);
      offset += length;
      formats.push_back(1); // All items are in binary format
    }
  }
};

template<typename... Ts>
class params : public params_base
{
  std::tuple<Ts...> params_;

public:
  params(Ts... args)
    : params_{ args... }
  {
  }

  void user_defined_type_names(std::vector<std::string_view>& vec, const detail::oid_map& omp)
  {
    detail::type_name_of<decltype(params_)>(vec, omp);
  }

  void do_serialize(std::string& buffer, std::vector<Oid>& types, std::vector<int>& lengths, const detail::oid_map& omp)
    const override
  {
    std::apply(
      [&](auto&&... param) { (add(buffer, types, lengths, omp, std::forward<decltype(param)>(param)), ...); }, params_);
  }
};

template<typename... Ts>
params(Ts... args) -> params<Ts...>;
} // namespace psql
