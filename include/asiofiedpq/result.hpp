#pragma once

#include <libpq-fe.h>

#include <memory>

namespace asiofiedpq
{
struct PGresultDeleter
{
  void operator()(PGresult* p)
  {
    PQclear(p);
  }
};

using result = std::unique_ptr<PGresult, PGresultDeleter>;
} // namespace asiofiedpq
