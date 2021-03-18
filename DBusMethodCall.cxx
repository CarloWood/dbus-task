#include "sys.h"
#include "DBusMethodCall.h"

namespace task {

char const* DBusMethodCall::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(DBusMethodCall_start);
    AI_CASE_RETURN(DBusMethodCall_done);
  }
  ASSERT(false);
  return "UNKNOWN STATE";
}

void DBusMethodCall::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case DBusMethodCall_start:
      break;
    case DBusMethodCall_done:
      finish();
      break;
  }
}

} // namespace task
