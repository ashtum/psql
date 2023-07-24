#pragma once

#include <boost/system/error_code.hpp>

namespace asiofiedpq
{
enum class error
{
  connection_failed = 1,
  pqstatus_failed,
  pqsetnonblocking_failed,
  pqenterpipelinemode_failed,
  pqsendqueryparams_failed,
  pqpipelinesync_failed,
};

inline const boost::system::error_category& error_category()
{
  struct category : boost::system::error_category
  {
    virtual ~category() = default;

    const char* name() const noexcept override
    {
      return "asiofiedpq";
    }

    std::string message(int ev) const override
    {
      switch (static_cast<error>(ev))
      {
        case error::connection_failed:
          return "Connection to database failed";
        case error::pqstatus_failed:
          return "PQstatus failed";
        case error::pqsetnonblocking_failed:
          return "PQsetnonblocking failed";
        case error::pqenterpipelinemode_failed:
          return "PQenterPipelineMode failed";
        case error::pqsendqueryparams_failed:
          return "PQsendQueryParams failed";
        case error::pqpipelinesync_failed:
          return "PQpipelineSync failed";
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
} // namespace asiofiedpq

namespace boost::system
{
template<>
struct is_error_code_enum<asiofiedpq::error>
{
  static bool const value = true;
};
} // namespace boost::system
