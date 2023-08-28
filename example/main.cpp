#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> async_main(std::string conninfo);

int main()
{
  try
  {
    auto ioc      = asio::io_context{};
    auto conninfo = std::string{ "postgresql://postgres:postgres@172.18.0.2:5432" };

    asio::co_spawn(
      ioc,
      async_main(conninfo),
      [](const std::exception_ptr& ep)
      {
        if (ep)
          std::rethrow_exception(ep);
      });

    ioc.run();
  }
  catch (const std::exception& e)
  {
    std::cout << "Exception: " << e.what() << std::endl;
  }
}
