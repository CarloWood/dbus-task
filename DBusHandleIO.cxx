#include "sys.h"
#include "DBusHandleIO.h"

namespace utils { using namespace threading; }
namespace task {

char const* DBusHandleIO::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(DBusHandleIO_start);
    AI_CASE_RETURN(DBusHandleIO_lock);
    AI_CASE_RETURN(DBusHandleIO_locked);
    AI_CASE_RETURN(DBusHandleIO_done);
  }
  AI_NEVER_REACHED;
}

void DBusHandleIO::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case DBusHandleIO_start:
    {
      //...
      set_state(DBusHandleIO_lock);
      wait(have_dbus_io);
      break;
    }
    case DBusHandleIO_lock:
      set_state(DBusHandleIO_locked);
      // Attempt to obtain the lock on the connection.
      if (!lock(this, connection_locked))
      {
        wait(connection_locked);
        break;
      }
      [[fallthrough]];
    case DBusHandleIO_locked:
    {
      set_state(DBusHandleIO_lock);
      statefultask::Lock lock(m_mutex);
      m_connection->handle_dbus_io();
      lock.unlock();
      wait(have_dbus_io);
      break;
    }
    case DBusHandleIO_done:
      finish();
      break;
  }
}

} // namespace task
