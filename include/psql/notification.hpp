#pragma once

#include <libpq-fe.h>

#include <memory>
#include <string_view>

namespace psql
{
class notification
{
  struct pgnotify_deleter
  {
    void operator()(PGnotify* p)
    {
      PQfreemem(p);
    }
  };

  std::unique_ptr<PGnotify, pgnotify_deleter> pg_notify_;

public:
  notification() = default;

  explicit notification(PGnotify* pg_notify)
    : pg_notify_{ pg_notify }
  {
  }

  operator bool() const noexcept
  {
    return !!pg_notify_;
  }

  int pid() const noexcept
  {
    if (pg_notify_)
      return pg_notify_->be_pid;

    return -1;
  }

  std::string_view channel() const noexcept
  {
    if (pg_notify_)
      return pg_notify_->relname;

    return {};
  }

  std::string_view payload() const noexcept
  {
    if (pg_notify_)
      return pg_notify_->extra;

    return {};
  }
};
} // namespace psql
