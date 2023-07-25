#pragma once

#include <libpq-fe.h>

#include <memory>

namespace asiofiedpq
{
struct pgresult_deleter
{
  void operator()(PGresult* p)
  {
    PQclear(p);
  }
};

using result = std::unique_ptr<PGresult, pgresult_deleter>;
} // namespace asiofiedpq
