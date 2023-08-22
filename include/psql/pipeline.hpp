#pragma once

#include <psql/error.hpp>
#include <psql/params.hpp>

#include <boost/system/system_error.hpp>

#include <libpq-fe.h>

namespace psql
{
class pipeline
{
  PGconn* pgconn_;
  size_t index_{};

public:
  pipeline(PGconn* pgconn)
    : pgconn_{ pgconn }
  {
  }

  pipeline(const pipeline&) = delete;
  pipeline(pipeline&&)      = delete;

  size_t push_query(const std::string& query, const params& params = {})
  {
    if (!PQsendQueryParams(
          pgconn_,
          query.data(),
          params.count(),
          params.types(),
          params.values(),
          params.lengths(),
          params.formats(),
          1))
      throw boost::system::system_error{ error::pq_send_query_params_failed };

    return index_++;
  }

  size_t push_query_prepared(const std::string& stmt_name, const params& params = {})
  {
    if (!PQsendQueryPrepared(
          pgconn_, stmt_name.data(), params.count(), params.values(), params.lengths(), params.formats(), 1))
      throw boost::system::system_error{ error::pq_send_query_prepared_failed };

    return index_++;
  }

  size_t size() const noexcept
  {
    return index_;
  }
};
} // namespace psql
