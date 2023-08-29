#pragma once

#include <psql/connection.hpp>

#include <boost/asio/steady_timer.hpp>

#include <mutex>
#include <queue>

namespace psql
{
namespace detail
{
template<typename Executor>
class basic_connection_pool_impl;
} // namespace detail

template<typename Executor = asio::any_io_executor>
class basic_pooled_connection
{
  using conn_pool_pointer_type = std::weak_ptr<detail::basic_connection_pool_impl<Executor>>;
  conn_pool_pointer_type conn_pool_;
  basic_connection<Executor> conn_;

public:
  explicit basic_pooled_connection(Executor exec)
    : conn_{ std::move(exec) }
  {
  }

  basic_pooled_connection(conn_pool_pointer_type conn_pool, basic_connection<Executor> conn)
    : conn_pool_{ std::move(conn_pool) }
    , conn_{ std::move(conn) }
  {
  }

  basic_pooled_connection(const basic_pooled_connection&)            = delete;
  basic_pooled_connection& operator=(const basic_pooled_connection&) = delete;

  basic_pooled_connection(basic_pooled_connection&&) noexcept = default;
  basic_pooled_connection& operator=(basic_pooled_connection&& other) noexcept
  {
    std::swap(conn_pool_, other.conn_pool_);
    std::swap(conn_, other.conn_);
    return *this;
  }

  basic_connection<Executor>* operator->() noexcept
  {
    return &conn_;
  }

  basic_connection<Executor>& operator*() noexcept
  {
    return conn_;
  }

  ~basic_pooled_connection();
};

namespace detail
{
template<typename Executor>
class basic_connection_pool_impl : public std::enable_shared_from_this<basic_connection_pool_impl<Executor>>
{
  using error_code = boost::system::error_code;

  std::mutex mtx_;
  Executor exec_;
  asio::steady_timer cv_;
  std::string conninfo_;
  size_t max_size_{};
  size_t aquired_conns_{};
  std::queue<basic_connection<Executor>> idle_conns_;

public:
  using executor_type = Executor;

  basic_connection_pool_impl(Executor exec, std::string conninfo, size_t max_size = 32)
    : exec_{ exec }
    , cv_{ std::move(exec), asio::steady_timer::time_point::max() }
    , conninfo_{ std::move(conninfo) }
    , max_size_{ max_size }
  {
  }

  executor_type get_executor() const noexcept
  {
    return exec_;
  }

  size_t max_size() noexcept
  {
    auto lg = std::lock_guard<std::mutex>{ mtx_ };
    return max_size_;
  }

  void max_size(size_t value) noexcept
  {
    auto lg   = std::lock_guard<std::mutex>{ mtx_ };
    max_size_ = value;
    cv_.cancel();
  }

  size_t num_aquired() noexcept
  {
    auto lg = std::lock_guard<std::mutex>{ mtx_ };
    return aquired_conns_;
  }

  template<typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_aquire(CompletionToken&& token = CompletionToken{})
  {
    return asio::async_compose<CompletionToken, void(error_code, basic_pooled_connection<executor_type>)>(
      [this, coro = asio::coroutine{}, conn = std::make_unique<basic_connection<Executor>>(exec_)](
        auto& self, error_code ec = {}) mutable
      {
        auto lg = std::unique_lock<std::mutex>{ mtx_ };

        BOOST_ASIO_CORO_REENTER(coro)
        {
          self.reset_cancellation_state(asio::enable_total_cancellation());

          while (aquired_conns_ >= max_size_)
          {
            if ((ec && ec != asio::error::operation_aborted) || !!self.cancelled())
            {
              lg.unlock();
              return self.complete(ec, { {}, std::move(*conn) });
            }

            BOOST_ASIO_CORO_YIELD cv_.async_wait(std::move(self));
          }

          aquired_conns_++;

          if (idle_conns_.empty())
          {
            BOOST_ASIO_CORO_YIELD conn->async_connect(conninfo_, std::move(self));
          }
          else
          {
            *conn = std::move(idle_conns_.front());
            idle_conns_.pop();
            BOOST_ASIO_CORO_YIELD asio::post(std::move(self));
          }

          lg.unlock();
          return self.complete(ec, { this->weak_from_this(), std::move(*conn) });
        }
      },
      token,
      exec_);
  }

  void return_connection(basic_connection<Executor>&& conn)
  {
    auto lg = std::lock_guard<std::mutex>{ mtx_ };

    aquired_conns_--;
    cv_.cancel_one();

    if (PQstatus(conn.native_handle()) != CONNECTION_OK)
      return;

    if (PQtransactionStatus(conn.native_handle()) != PQTRANS_IDLE)
      return;

    idle_conns_.push(std::move(conn));
  }
};
} // namespace detail

template<typename Executor>
basic_pooled_connection<Executor>::~basic_pooled_connection()
{
  if (auto sp = conn_pool_.lock())
    sp->return_connection(std::move(conn_));
}

template<typename Executor = asio::any_io_executor>
class basic_connection_pool
{
  using impl_type = detail::basic_connection_pool_impl<Executor>;
  std::shared_ptr<impl_type> impl_;

public:
  using executor_type = Executor;

  basic_connection_pool(Executor exec, std::string conninfo, size_t max_size = 32)
    : impl_{ std::make_shared<impl_type>(std::move(exec), std::move(conninfo), max_size) }
  {
  }

  template<typename OtherExecutor>
  struct rebind_executor
  {
    using other = basic_connection_pool<OtherExecutor>;
  };

  executor_type get_executor() const noexcept
  {
    return impl_->get_executor();
  }

  size_t max_size() const noexcept
  {
    return impl_->max_size();
  }

  void max_size(size_t value) noexcept
  {
    impl_->max_size(value);
  }

  size_t num_aquired() const noexcept
  {
    return impl_->num_aquired();
  }

  template<typename CompletionToken = asio::default_completion_token_t<executor_type>>
  auto async_aquire(CompletionToken&& token = CompletionToken{})
  {
    return impl_->async_aquire(std::forward<CompletionToken>(token));
  }
};

using connection_pool   = basic_connection_pool<>;
using pooled_connection = basic_pooled_connection<>;
} // namespace psql
