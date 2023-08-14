#pragma once

#include <boost/system/error_code.hpp>

namespace psql
{
enum class error
{
  connection_failed = 1,
  pq_status_failed,
  pq_set_non_blocking_failed,
  pq_enter_pipeline_mode_failed,
  pq_send_query_params_failed,
  pq_send_prepare_failed,
  pq_send_query_prepared_failed,
  pq_send_describe_prepared_failed,
  pq_send_describe_portal_failed,
  pq_pipeline_sync_failed,
  pq_consume_input_failed
};

inline const boost::system::error_category& error_category()
{
  struct category : boost::system::error_category
  {
    virtual ~category() = default;

    const char* name() const noexcept override
    {
      return "psql";
    }

    std::string message(int ev) const override
    {
      switch (static_cast<error>(ev))
      {
        case error::connection_failed:
          return "Connection to database failed";
        case error::pq_status_failed:
          return "PQstatus failed, check error message on connection";
        case error::pq_set_non_blocking_failed:
          return "PQsetnonblocking failed, check error message on connection";
        case error::pq_enter_pipeline_mode_failed:
          return "PQenterPipelineMode failed, check error message on connection";
        case error::pq_send_query_params_failed:
          return "PQsendQueryParams failed, check error message on connection";
        case error::pq_send_prepare_failed:
          return "PQsendPrepare failed, check error message on connection";
        case error::pq_send_query_prepared_failed:
          return "PQsendQueryPrepared failed, check error message on connection";
        case error::pq_send_describe_prepared_failed:
          return "PQsendDescribePrepared failed, check error message on connection";
        case error::pq_send_describe_portal_failed:
          return "PQsendDescribePortal failed, check error message on connection";
        case error::pq_pipeline_sync_failed:
          return "PQpipelineSync failed, check error message on connection";
        case error::pq_consume_input_failed:
          return "PQconsumeInput failed, check error message on connection";
        default:
          return "Unknown error";
      }
    }
  };

  static const auto category_ = category{};

  return category_;
};

inline boost::system::error_code make_error_code(error e)
{
  return { static_cast<int>(e), error_category() };
}
} // namespace psql

namespace boost::system
{
template<>
struct is_error_code_enum<psql::error>
{
  static const bool value = true;
};
} // namespace boost::system
