#include "sys.h"
#include "systemd_sd-bus.h"
#include "DBusMethodCall.h"

namespace utils { using namespace threading; }
namespace task {

char const* DBusMethodCall::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(DBusMethodCall_start);
    AI_CASE_RETURN(DBusMethodCall_lock);
    AI_CASE_RETURN(DBusMethodCall_locked);
    AI_CASE_RETURN(DBusMethodCall_done);
  }
  AI_NEVER_REACHED;
}

void DBusMethodCall::reply_callback(dbus::MessageRead const& message)
{
  DoutEntering(dc::notice, "DBusMethodCall::reply_callback()");
  // This is a callback from sd_bus, so we have the lock on the connection.
  m_reply_callback(message);
  // We're done with the message.
  m_message.reset();
  // Unlock the mutex before waking up the task.
  m_dbus_connection->unlock(true);
  signal(have_reply_callback);
}

void DBusMethodCall::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case DBusMethodCall_start:
    {
      m_dbus_connection = m_broker->run(*m_broker_key, [this](bool success){ Dout(dc::notice, "dbus_connection finished!"); signal(connection_set_up); });
      Dout(dc::notice, "Requested name = \"" << m_dbus_connection->service_name() << "\".");
      set_state(DBusMethodCall_lock);
      wait(connection_set_up);
      break;
    }
    case DBusMethodCall_lock:
      set_state(DBusMethodCall_locked);
      // Attempt to obtain the lock on the connection.
      if (!m_dbus_connection->lock(this, connection_locked))
      {
        wait(connection_locked);
        break;
      }
      [[fallthrough]];
    case DBusMethodCall_locked:
    {
      set_state(DBusMethodCall_done);
      DBusLock lock(m_dbus_connection);
      Dout(dc::notice, "Unique name = \"" << m_dbus_connection->get_unique_name() << "\".");
      m_message.create_message(m_dbus_connection, *m_destination);
      m_params_callback(m_message);
      int res = sd_bus_call_async(m_dbus_connection->get_bus(), nullptr, m_message, &DBusMethodCall::reply_callback, this, 0);
      lock.unlock();
      if (res < 0)
        THROW_ALERTC(-res, "sd_bus_call_async");
      wait(have_reply_callback);
      break;
    }
    case DBusMethodCall_done:
      finish();
      break;
  }
}

void DBusMethodCall::abort_impl()
{
  // We can't unref m_message during destruction, because then we can't take the lock on m_dbus_connection.
  // Therefore do that here.
  if (AI_UNLIKELY(m_dbus_connection))     // Could be aborted before it even got the chance to run DBusMethodCall_start.
  {
    m_dbus_connection->lock_blocking(this);
    DBusLock lock(m_dbus_connection);
    m_message.reset();
  }
}

} // namespace task
