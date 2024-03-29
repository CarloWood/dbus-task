#include "sys.h"
#include "DBusHandleIO.h"

namespace utils { using namespace threading; }
namespace task {

char const* DBusHandleIO::condition_str_impl(condition_type condition) const
{
  switch (condition)
  {
    AI_CASE_RETURN(have_dbus_io);
    AI_CASE_RETURN(connection_locked);
  }
  return direct_base_type::condition_str_impl(condition);
}

char const* DBusHandleIO::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(DBusHandleIO_start);
    AI_CASE_RETURN(DBusHandleIO_wait_for_lock);
    AI_CASE_RETURN(DBusHandleIO_locked);
    AI_CASE_RETURN(DBusHandleIO_done);
  }
  AI_NEVER_REACHED;
}

char const* DBusHandleIO::task_name_impl() const
{
  return "DBusHandleIO";
}

void DBusHandleIO::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case DBusHandleIO_start:
    {
      m_connection->handle_io_ready();  // We are ready to receive signals now.
      set_state(DBusHandleIO_wait_for_lock);
      wait(have_dbus_io);
      break;
    }
    case DBusHandleIO_wait_for_lock:
      set_state(DBusHandleIO_locked);
      // Attempt to obtain the lock on the connection.
      if (!lock(this, connection_locked))
      {
        wait(connection_locked);
        break;
      }
      break;
    case DBusHandleIO_locked:
    {
      obtained_lock();
      set_state(DBusHandleIO_wait_for_lock);
      statefultask::AdoptLock scoped_lock(m_mutex);
      switch (m_connection->handle_dbus_io())
      {
        case dbus::Connection::needs_relock:
          scoped_lock.skip_unlock();
          break;
        case dbus::Connection::unlocked_and_io_handled:
          scoped_lock.skip_unlock();
          wait(have_dbus_io);
          break;
        case dbus::Connection::io_handled:
          scoped_lock.unlock();
          wait(have_dbus_io);
          break;
      }
      break;
    }
    case DBusHandleIO_done:
      finish();
      break;
  }
}

} // namespace task
