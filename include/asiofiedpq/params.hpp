#pragma once

#include <asiofiedpq/detail/serialization.hpp>

#include <libpq-fe.h>

#include <array>
#include <memory>

namespace asiofiedpq
{
class params
{
  class interface
  {
  public:
    virtual int count() const                 = 0;
    virtual const ::Oid* types() const        = 0;
    virtual const char* const* values() const = 0;
    virtual const int* lengths() const        = 0;
    virtual const int* formats() const        = 0;
    virtual ~interface()                      = default;
  };
  std::unique_ptr<interface> impl_;
  static const inline oid_map empty_omp;

public:
  params() = default;

  template<typename... Ts>
  params(const oid_map& omp, Ts&&... params)
    requires(!(std::is_same_v<asiofiedpq::params, std::decay_t<Ts>> || ...))
    : impl_{ std::make_unique<impl<sizeof...(Ts)>>(omp, std::forward<Ts>(params)...) }
  {
  }

  template<typename... Ts>
  params(Ts&&... params)
    requires(
      !((std::is_same_v<oid_map, std::decay_t<Ts>> || std::is_same_v<asiofiedpq::params, std::decay_t<Ts>>) || ...))
    : impl_{ std::make_unique<impl<sizeof...(Ts)>>(empty_omp, std::forward<Ts>(params)...) }
  {
  }

  int count() const
  {
    return impl_ ? impl_->count() : 0;
  }

  const ::Oid* types() const
  {
    return impl_ ? impl_->types() : nullptr;
  }

  const char* const* values() const
  {
    return impl_ ? impl_->values() : nullptr;
  }

  const int* lengths() const
  {
    return impl_ ? impl_->lengths() : nullptr;
  }

  const int* formats() const
  {
    return impl_ ? impl_->formats() : nullptr;
    ;
  }

private:
  template<size_t N>
  class impl : public interface
  {
    std::string buffer_;
    std::array<::Oid, N> types_;
    std::array<const char*, N> values_;
    std::array<int, N> lengths_;
    std::array<int, N> formats_;

  public:
    template<typename... Params>
    impl(const oid_map& omp, Params&&... params)
    {
      [&]<size_t... Is>(std::index_sequence<Is...>)
      { (add(omp, Is, std::forward<Params>(params)), ...); }(std::index_sequence_for<Params...>());

      // Converts offsets to pointers
      for (size_t offset = 0, i = 0; i < N; i++)
      {
        values_[i] = lengths_[i] ? buffer_.data() + offset : nullptr;
        offset += lengths_[i];
      }

      formats_.fill(1); // All items are in binary format
    }

    impl(const impl&) = delete;
    impl(impl&&)      = delete;

    int count() const override
    {
      return N;
    }

    const ::Oid* types() const override
    {
      return types_.data();
    }

    const char* const* values() const override
    {
      return values_.data();
    }

    const int* lengths() const override
    {
      return lengths_.data();
    }

    const int* formats() const override
    {
      return formats_.data();
    }

  private:
    template<typename T>
    void add(const oid_map& omp, size_t i, const T& value)
    {
      types_[i]   = detail::oid_of<std::decay_t<T>>(omp);
      lengths_[i] = detail::size_of(value);
      detail::serialize(omp, &buffer_, value);
    }
  };
};
} // namespace asiofiedpq
