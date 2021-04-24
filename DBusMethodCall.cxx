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
    AI_CASE_RETURN(DBusMethodCall_create_message);
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
  m_dbus_connection->unlock();
  signal(2);
}

void DBusMethodCall::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case DBusMethodCall_start:
    {
      m_dbus_connection = m_broker->run(*m_broker_key, [this](bool success){ Dout(dc::notice, "dbus_connection finished!"); signal(1); });
      Dout(dc::notice, "Requested name = \"" << m_dbus_connection->service_name() << "\".");
      set_state(DBusMethodCall_create_message);
      wait(1);
      break;
    }
    case DBusMethodCall_create_message:
    {
      DBusMutex m{m_dbus_connection};
      std::unique_lock<DBusMutex> lk(m);
      Dout(dc::notice, "Unique name = \"" << m_dbus_connection->get_unique_name() << "\".");
      m_message.create_message(m_dbus_connection, *m_destination);
      m_params_callback(m_message);
      int res = sd_bus_call_async(m_dbus_connection->get_bus(), nullptr, m_message, &DBusMethodCall::reply_callback, this, 0);
      lk.unlock();
      if (res < 0)
        THROW_ALERTC(-res, "sd_bus_call_async");
      set_state(DBusMethodCall_done);
      wait(2);
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
    DBusMutex m{m_dbus_connection};
    std::unique_lock<DBusMutex> lk(m);
    m_message.reset();
  }
}

} // namespace task
