#include <psql/connection.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/deferred.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> run_exmaple(psql::connection& conn)
{
  co_await conn.async_query("LISTEN channel_a;", asio::deferred);
  co_await conn.async_query("LISTEN channel_b;", asio::deferred);

  co_await conn.async_query("NOTIFY channel_a, 'payload_1';", asio::deferred);
  co_await conn.async_query("NOTIFY channel_b, 'payload_1';", asio::deferred);
  co_await conn.async_query("NOTIFY channel_a, 'payload_2';", asio::deferred);
  co_await conn.async_query("NOTIFY channel_b, 'payload_2';", asio::deferred);

  for (int i = 0; i < 4; i++)
  {
    auto notif = co_await conn.async_receive_notifcation(asio::deferred);
    std::cout << notif.pid() << "\t" << notif.channel() << "\t" << notif.payload() << std::endl;
  }
}
