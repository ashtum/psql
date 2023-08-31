#include <psql/connection.hpp>
#include <psql/connection_pool.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/detached.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> child_task(psql::connection_pool conn_pool)
{
  // Acquire a connection from the connection pool. If the number of acquired connections reaches the max_size, this
  // operation will wait until a connection is returned to the connection pool.
  auto conn = co_await conn_pool.async_aquire(asio::deferred);

  // Hold the connection for 3 seconds for demonstration purposes.
  co_await conn->async_query("SELECT pg_sleep(3);", asio::deferred);

  // Retrieve the count of active connections on the server.
  auto active_conns = co_await conn->async_query("SELECT sum(numbackends) FROM pg_stat_database;", asio::deferred);
  std::cout << "active connections:" << as<int64_t>(active_conns) << std::endl;

  // The destructor of pooled_connection will automatically return the connection
  // to the pool if the pool is still active.
}

asio::awaitable<void> async_main(std::string conninfo)
{
  auto exec = co_await asio::this_coro::executor;
  // Create a connection_pool with a max_size of 4.
  auto conn_pool = psql::connection_pool{ exec, conninfo, 4 };

  // Instances of connection_pool are inexpensive to copy (as they are handlers to a shared object).
  for (auto i = 0; i < 8; i++)
    asio::co_spawn(exec, child_task(conn_pool), asio::detached);

  // Since we passed copies of connection_pool to the child tasks, we can safely exit this coroutine and allow our
  // instance of conn_pool to be destroyed.
}
