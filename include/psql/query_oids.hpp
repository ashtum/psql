#include <psql/connection.hpp>

namespace psql
{
auto async_query_oids(
  connection& conn,
  oid_map& omp,
  asio::completion_token_for<void(boost::system::error_code)> auto&& token)
{
  return asio::async_compose<decltype(token), void(boost::system::error_code)>(
    [coro = asio::coroutine{}, conn = &conn, omp = &omp](
      auto& self, boost::system::error_code ec = {}, result result = {}) mutable
    {
      if (ec)
        return self.complete(ec);

      BOOST_ASIO_CORO_REENTER(coro)
      {
        BOOST_ASIO_CORO_YIELD conn->async_query(
          R"(
            SELECT
              type_name,
              COALESCE(to_regtype(type_name)::oid, - 1),
              COALESCE(to_regtype(type_name || '[]')::oid, - 1) 
            FROM
              UNNEST($1) As type_name
          )",
          omp->get_type_names(),
          std::move(self));

        // TODO handle exceptions
        for (auto row : result)
        {
          auto [name, type_oid, array_oid] = as<std::string_view, uint32_t, uint32_t>(row);
          omp->set_type_oids(name, type_oid, array_oid);
        }

        self.complete({});
      }
    },
    token);
}
} // namespace psql
