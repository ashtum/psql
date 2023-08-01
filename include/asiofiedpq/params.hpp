#pragma once

#include <asiofiedpq/oid_map.hpp>

#include <boost/endian.hpp>

#include <libpq-fe.h>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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
    using order = boost::endian::order;

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
    void add(size_t i, const std::vector<Param>& param)
      requires(std::is_integral_v<Param> || std::is_floating_point_v<Param> || std::is_same_v<Param, std::byte>)
    {
      const auto init_size = buffer_.size();
      boost::endian::endian_store<int32_t, 4, order::big>(make_buffer(4), 1);
      boost::endian::endian_store<int32_t, 4, order::big>(make_buffer(4), 0);
      boost::endian::endian_store<int32_t, 4, order::big>(make_buffer(4), oid_map<Param>::value);
      boost::endian::endian_store<int32_t, 4, order::big>(make_buffer(4), std::size(param));
      boost::endian::endian_store<int32_t, 4, order::big>(make_buffer(4), 0);

      for (const auto& item : param)
      {
        boost::endian::endian_store<int32_t, 4, order::big>(make_buffer(4), sizeof(item));
        boost::endian::endian_store<Param, sizeof(item), order::big>(make_buffer(sizeof(item)), item);
      }

      types_[i]   = oid_map<Param>::arr_value;
      lengths_[i] = static_cast<int>(buffer_.size() - init_size);
    }

    template<typename Param>
    void add(size_t i, Param param)
      requires(std::is_integral_v<Param> || std::is_floating_point_v<Param>)
    {
      boost::endian::endian_store<Param, sizeof(Param), order::big>(make_buffer(sizeof(Param)), param);
      types_[i]   = oid_map<Param>::value;
      lengths_[i] = sizeof(Param);
    }

    void add(size_t i, std::nullptr_t)
    {
      types_[i]   = 0; // Oid of NULL
      lengths_[i] = 0;
    }

    template<typename Param>
    void add(size_t i, const std::optional<Param>& opt)
    {
      opt ? add(i, *opt) : add(i, nullptr);
    }

    template<typename Param>
    void add(size_t i, Param* param)
    {
      param ? add(i, *param) : add(i, nullptr);
    }

    void add(size_t i, std::string_view param)
    {
      buffer_.append(param);
      types_[i]   = 25; // Oid of text
      lengths_[i] = param.size();
    }

    void add(size_t i, const char* param)
    {
      add(i, std::string_view{ param });
    }

    void add(size_t i, std::chrono::system_clock::time_point param)
    {
      auto pg_epoch = std::chrono::system_clock::time_point{} + std::chrono::seconds{ 946684800 };
      int64_t value = std::chrono::duration_cast<std::chrono::microseconds>(param - pg_epoch).count();
      boost::endian::endian_store<int64_t, sizeof(int64_t), order::big>(make_buffer(sizeof(int64_t)), value);
      types_[i]   = oid_map<std::chrono::system_clock::time_point>::value;
      lengths_[i] = sizeof(int64_t);
    }

    uint8_t* make_buffer(size_t size)
    {
      buffer_.resize(buffer_.size() + size);
      return reinterpret_cast<uint8_t*>(std::addressof(buffer_.back()) - size + 1);
    }
  };
};
} // namespace asiofiedpq
