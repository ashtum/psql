#include <psql/connection.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/experimental/promise.hpp>
#include <boost/asio/experimental/use_promise.hpp>
#include <boost/asio/steady_timer.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> notifcation_receiver(psql::connection& conn)
{
  for (;;)
  {
    // `async_receive_notification` is the only asynchronous operation that can be initiated while other active
    // asynchronous operations are ongoing on the connection.
    auto notif = co_await conn.async_receive_notifcation(asio::deferred);

    std::cout << "Channel:" << notif.channel() << "\tPayload:" << notif.payload() << std::endl;

    if (notif.payload() == "10")
      break;
  }
}

asio::awaitable<void> async_main(std::string conninfo)
{
  auto exec = co_await asio::this_coro::executor;
  auto conn = psql::connection{ exec };

  co_await conn.async_connect(conninfo, asio::deferred);
  co_await conn.async_query("LISTEN counter;", asio::deferred);

  auto receiver = asio::co_spawn(exec, notifcation_receiver(conn), asio::experimental::use_promise);
  auto timer    = asio::steady_timer{ exec };

  for (auto i = 0; i <= 10; i++)
  {
    timer.expires_after(std::chrono::seconds{ 1 });
    co_await timer.async_wait(asio::deferred);

    co_await conn.async_query("SELECT pg_notify('counter', $1::TEXT);", psql::mp(i), asio::deferred);
  }

  co_await receiver(asio::deferred);
}
