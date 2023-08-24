#pragma once

#include <psql/detail/serialization.hpp>
#include <psql/detail/type_name_of.hpp>

#include <libpq-fe.h>

#include <memory>

namespace psql
{
class params
{
  struct interface
  {
    virtual void user_defined_type_names(std::vector<std::string_view>&, const detail::oid_map&) const       = 0;
    virtual void serialize(std::string&, std::vector<Oid>&, std::vector<int>&, const detail::oid_map&) const = 0;
    virtual ~interface()                                                                                     = default;
  };

  std::unique_ptr<interface> impl_;

public:
  params() = default;

  template<typename... Ts>
  params(Ts&&... args)
    requires(!(std::is_same_v<psql::params, std::decay_t<Ts>> || ...))
    : impl_{ std::make_unique<impl<Ts...>>(args...) }
  {
  }

  void user_defined_type_names(std::vector<std::string_view>& vec, const detail::oid_map& omp) const
  {
    if (!impl_)
      return;

    impl_->user_defined_type_names(vec, omp);
  }

  void serialize(
    std::string& buffer,
    std::vector<Oid>& types,
    std::vector<int>& lengths,
    std::vector<const char*>& values,
    std::vector<int>& formats,
    const detail::oid_map& omp) const
  {
    if (!impl_)
      return;

    buffer.clear();
    types.clear();
    lengths.clear();
    values.clear();
    formats.clear();

    impl_->serialize(buffer, types, lengths, omp);

    for (size_t offset = 0; const auto& length : lengths)
    {
      values.push_back(length ? buffer.data() + offset : nullptr);
      offset += length;
      formats.push_back(1); // All items are in binary format
    }
  }

private:
  template<typename... Ts>
  struct impl : interface
  {
    std::tuple<Ts...> params;

    impl(Ts... args)
      : params{ args... }
    {
    }

    void user_defined_type_names(std::vector<std::string_view>& vec, const detail::oid_map& omp) const override
    {
      detail::type_name_of<decltype(params)>(vec, omp);
    }

    void serialize(std::string& buffer, std::vector<Oid>& types, std::vector<int>& lengths, const detail::oid_map& omp)
      const override
    {
      std::apply(
        [&](auto&&... param) { (add(buffer, types, lengths, omp, std::forward<decltype(param)>(param)), ...); },
        params);
    }
  };

  template<typename T>
  static void
  add(std::string& buffer, std::vector<Oid>& types, std::vector<int>& lengths, const detail::oid_map& omp, T&& value)
  {
    types.push_back(detail::oid_of<std::decay_t<T>>(omp));
    lengths.push_back(detail::size_of<std::decay_t<T>>(value));
    detail::serialize<std::decay_t<T>>(omp, &buffer, value);
  }

  static void add(
    std::string& buffer,
    std::vector<Oid>& types,
    std::vector<int>& lengths,
    const detail::oid_map& omp,
    const char* value)
  {
    types.push_back(detail::oid_of<const char*>(omp));
    lengths.push_back(detail::size_of<const char*>(value));
    detail::serialize<const char*>(omp, &buffer, value);
  }
};
} // namespace psql
