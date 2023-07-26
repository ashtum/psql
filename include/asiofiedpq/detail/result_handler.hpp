#pragma once

#include <asiofiedpq/result.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/steady_timer.hpp>

namespace asiofiedpq
{
namespace detail
{
namespace asio = boost::asio;

class result_handler
{
  enum class status
  {
    waiting,
    completed,
    cancelled
  };

  status status_{ status::waiting };
  asio::steady_timer cv_;

public:
  explicit result_handler(asio::any_io_executor exec)
    : cv_{ exec, asio::steady_timer::time_point::max() }
  {
  }

  bool is_waiting() const noexcept
  {
    return status_ == status::waiting;
  }

  bool is_completed() const noexcept
  {
    return status_ == status::completed;
  }

  bool is_cancelled() const noexcept
  {
    return status_ == status::cancelled;
  }

  auto async_wait(asio::completion_token_for<void(boost::system::error_code)> auto&& token)
  {
    return cv_.async_wait(token);
  }

  void cancel()
  {
    status_ = status::cancelled;
    cv_.cancel_one();
  }

  void complete()
  {
    status_ = status::completed;
    cv_.cancel_one();
  }

  virtual void handle(result) = 0;

  virtual ~result_handler() = default;
};
} // namespace detail
} // namespace asiofiedpq
