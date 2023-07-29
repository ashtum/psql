#pragma once

#include <asiofiedpq/oid_map.hpp>

#include <boost/endian.hpp>

#include <libpq-fe.h>

#include <array>
#include <memory>
#include <string>
#include <string_view>

namespace asiofiedpq
{
class params
{
  class interface
  {
  public:
    virtual int count() const                 = 0;
    virtual const Oid* types() const          = 0;
    virtual const char* const* values() const = 0;
    virtual const int* lengths() const        = 0;
    virtual const int* formats() const        = 0;
    virtual ~interface()                      = default;
  };
  std::unique_ptr<interface> impl_;

public:
  params() = default;

  params(auto&&... params)
    requires(!(std::is_same_v<asiofiedpq::params, std::decay_t<decltype(params)>> || ...))
    : impl_{ std::make_unique<impl<sizeof...(params)>>(std::forward<decltype(params)>(params)...) }
  {
  }

  int count() const
  {
    return impl_ ? impl_->count() : 0;
  }

  const Oid* types() const
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
    static constexpr auto binary_format = 1;

    std::string buffer_;
    std::array<Oid, N> types_;
    std::array<const char*, N> values_;
    std::array<int, N> lengths_;
    std::array<int, N> formats_;

  public:
    template<typename... Params>
    impl(Params&&... params)
    {
      [&]<size_t... Is>(std::index_sequence<Is...>)
      { (add(Is, std::forward<Params>(params)), ...); }(std::index_sequence_for<Params...>());

      // convert offsets to pointers
      for (size_t offset = 0, i = 0; i < N; i++)
      {
        values_[i] = buffer_.data() + offset;
        offset += lengths_[i];
      }
    }

    impl(const impl&) = delete;
    impl(impl&&)      = delete;

    int count() const override
    {
      return N;
    }

    const Oid* types() const override
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
    template<typename Param>
    void add(size_t i, Param param)
      requires(std::is_integral_v<Param> || std::is_floating_point_v<Param>)
    {
      boost::endian::endian_store<Param, sizeof(Param), boost::endian::order::big>(make_buffer(sizeof(Param)), param);
      types_[i]   = oid_map<Param>::value;
      lengths_[i] = sizeof(Param);
      formats_[i] = binary_format;
    }

    void add(size_t i, std::string_view param)
    {
      buffer_.append(param);
      types_[i]   = 25; // Oid of text
      lengths_[i] = param.size();
      formats_[i] = binary_format;
    }

    uint8_t* make_buffer(size_t size)
    {
      buffer_.resize(buffer_.size() + size);
      return reinterpret_cast<uint8_t*>(std::addressof(buffer_.back()) - size + 1);
    }
  };
};
} // namespace asiofiedpq
