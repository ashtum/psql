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
  params(auto&&... params)
    requires(!(std::is_same_v<asiofiedpq::params, std::decay_t<decltype(params)>> || ...))
    : impl_{ std::make_unique<impl<sizeof...(params)>>(std::forward<decltype(params)>(params)...) }
  {
  }

  int count() const
  {
    return impl_->count();
  }

  const Oid* types() const
  {
    return impl_->types();
  }

  const char* const* values() const
  {
    return impl_->values();
  }

  const int* lengths() const
  {
    return impl_->lengths();
  }

  const int* formats() const
  {
    return impl_->formats();
    ;
  }

private:
  template<size_t N>
  class impl : public interface
  {
    static constexpr auto BINARY_FORMAT = 1;

    std::string buffer_;
    std::array<Oid, N> types_;
    std::array<const char*, N> values_;
    std::array<int, N> lengths_;
    std::array<int, N> formats_;

  public:
    impl(auto&&... params)
    {
      [&]<size_t... Is>(std::index_sequence<Is...>)
      { (add(Is, std::forward<decltype(params)>(params)), ...); }(std::index_sequence_for<decltype(params)...>());

      // convert offsets to pointers
      for (auto& offset : values_)
        offset = std::next(buffer_.data(), reinterpret_cast<size_t>(offset));
    }

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
      ;
    }

  private:
    template<typename Param>
    void add(size_t i, Param param)
      requires(std::is_integral_v<Param> || std::is_floating_point_v<Param>)
    {
      store_current_offset(i);
      boost::endian::endian_store<Param, sizeof(Param), boost::endian::order::big>(
        reinterpret_cast<unsigned char*>(make_buffer(sizeof(Param))), param);
      types_[i]   = oid_map<Param>::value;
      lengths_[i] = sizeof(Param);
      formats_[i] = BINARY_FORMAT;
    }

    void add(size_t i, std::string_view param)
    {
      store_current_offset(i);
      buffer_.append(param);
      types_[i]   = 25; // Oid of text
      lengths_[i] = param.size();
      formats_[i] = BINARY_FORMAT;
    }

    char* make_buffer(size_t size)
    {
      buffer_.resize(buffer_.size() + size);
      return std::addressof(buffer_.back()) - size + 1;
    }

    void store_current_offset(size_t i)
    {
      values_[i] = reinterpret_cast<const char*>(buffer_.size());
    }
  };
};
} // namespace asiofiedpq
