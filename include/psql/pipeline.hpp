#pragma once

#include <psql/detail/oid_map.hpp>
#include <psql/detail/serialization.hpp>
#include <psql/error.hpp>

#include <boost/system/system_error.hpp>

#include <libpq-fe.h>

namespace psql
{
class pipeline
{
  PGconn* pgconn_;
  detail::oid_map& oid_map_;
  std::string& buffer_;
  size_t index_{};

public:
  pipeline(PGconn* pgconn, detail::oid_map& oid_map, std::string& buffer)
    : pgconn_{ pgconn }
    , oid_map_{ oid_map }
    , buffer_{ buffer }
  {
  }

  pipeline(const pipeline&) = delete;
  pipeline(pipeline&&)      = delete;

  template<typename... Ts>
  size_t push_query(const std::string& query, params<Ts...> params = {})
  {
    auto [t, v, l, f] = detail::serialize(oid_map_, buffer_, params);

    if (!PQsendQueryParams(pgconn_, query.data(), t.size(), t.data(), v.data(), l.data(), f.data(), 1))
      throw boost::system::system_error{ error::pq_send_query_params_failed };

    return index_++;
  }

  template<typename... Ts>
  size_t push_query_prepared(const std::string& stmt_name, params<Ts...> params = {})
  {
    auto [t, v, l, f] = detail::serialize(oid_map_, buffer_, params);

    if (!PQsendQueryPrepared(pgconn_, stmt_name.data(), t.size(), v.data(), l.data(), f.data(), 1))
      throw boost::system::system_error{ error::pq_send_query_prepared_failed };

    return index_++;
  }

  size_t size() const noexcept
  {
    return index_;
  }
};
} // namespace psql
