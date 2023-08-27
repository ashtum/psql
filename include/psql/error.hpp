#pragma once

#include <boost/system/error_code.hpp>

namespace psql
{
enum class error
{
  connection_failed = 1,
  pq_status_failed,
  pq_set_non_blocking_failed,
  pq_flush_failed,
  pq_enter_pipeline_mode_failed,
  pq_exit_pipeline_mode_failed,
  pq_send_query_params_failed,
  pq_send_prepare_failed,
  pq_send_query_prepared_failed,
  pq_send_describe_prepared_failed,
  pq_send_describe_portal_failed,
  pq_pipeline_sync_failed,
  pq_consume_input_failed,
  result_status_bad_response,
  result_status_empty_query,
  result_status_fatal_error,
  result_status_pipeline_aborted,
  result_status_unexpected,
  unexpected_non_null_result,
  exception_in_pipeline_operation,
  user_defined_type_does_not_exist,
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
          return "PQstatus failed, check the error message on the connection";
        case error::pq_set_non_blocking_failed:
          return "PQsetnonblocking failed, check the error message on the connection";
        case error::pq_flush_failed:
          return "PQflush failed, check the error message on the connection";
        case error::pq_enter_pipeline_mode_failed:
          return "PQenterPipelineMode failed, check the error message on the connection";
        case error::pq_exit_pipeline_mode_failed:
          return "PQexitPipelineMode failed, check the error message on the connection";
        case error::pq_send_query_params_failed:
          return "PQsendQueryParams failed, check the error message on the connection";
        case error::pq_send_prepare_failed:
          return "PQsendPrepare failed, check the error message on the connection";
        case error::pq_send_query_prepared_failed:
          return "PQsendQueryPrepared failed, check the error message on the connection";
        case error::pq_send_describe_prepared_failed:
          return "PQsendDescribePrepared failed, check the error message on the connection";
        case error::pq_send_describe_portal_failed:
          return "PQsendDescribePortal failed, check the error message on the connection";
        case error::pq_pipeline_sync_failed:
          return "PQpipelineSync failed, check the error message on the connection";
        case error::pq_consume_input_failed:
          return "PQconsumeInput failed, check the error message on the connection";
        case error::result_status_bad_response:
          return "The server's response was not understood";
        case error::result_status_empty_query:
          return "The query sent to the server was empty";
        case error::result_status_fatal_error:
          return "Fatal error in query execution, check the error message on the result";
        case error::result_status_pipeline_aborted:
          return "Pipeline execution aborted, check the error message on the result";
        case error::result_status_unexpected:
          return "Unexpected status from query result";
        case error::unexpected_non_null_result:
          return "Unexpected non null result";
        case error::exception_in_pipeline_operation:
          return "An exception occurred while executing the pipeline operation";
        case error::user_defined_type_does_not_exist:
          return "No user-defined type with the given name was found on the server";
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
