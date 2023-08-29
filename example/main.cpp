#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> async_main(std::string conninfo);

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    std::cerr << "Usage: ./executable <connection-uri>\n";
    std::cerr << "Example:\n";
    std::cerr << "    ./pipeline postgresql://user:secret@localhost:5433\n";
    return EXIT_FAILURE;
  }

  try
  {
    auto ioc = asio::io_context{};

    asio::co_spawn(
      ioc,
      async_main(argv[1]),
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
