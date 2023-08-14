#include <psql/connection.hpp>
#include <psql/detail/deserialization.hpp>

namespace psql
{
auto async_query_oids(
  connection& conn,
  oid_map& omp,
  asio::completion_token_for<void(boost::system::error_code)> auto&& token)
{
  return asio::async_initiate<decltype(token), void(boost::system::error_code)>(
    asio::experimental::co_composed<void(boost::system::error_code)>(
      [](auto state, connection* conn, oid_map* omp) -> void
      {
        state.on_cancellation_complete_with(asio::error::operation_aborted);

        const auto type_names   = omp->get_type_names();
        const auto [ec, result] = co_await conn->async_query(
          "SELECT t, COALESCE(to_regtype(t)::oid, -1), COALESCE(to_regtype(t || '[]')::oid, -1) FROM UNNEST($1) As t",
          type_names,
          asio::as_tuple(asio::deferred));

        for (const auto row : result)
        {
          const auto [name, type_oid, array_oid] = as<std::string_view, uint32_t, uint32_t>(row);
          omp->set_type_oids(name, type_oid, array_oid);
        }

        co_return ec;
      },
      conn.get_executor()),
    token,
    &conn,
    &omp);
}
} // namespace psql
