#include "sys.h"
#include <systemd/sd-bus.h>
#include "DBusMatchSignal.h"

namespace task {

char const* DBusMatchSignal::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(DBusMatchSignal_start);
    AI_CASE_RETURN(DBusMatchSignal_create_message);
    AI_CASE_RETURN(DBusMatchSignal_done);
  }
  AI_NEVER_REACHED;
}

void DBusMatchSignal::match_callback(dbus::MessageRead const& message)
{
  DoutEntering(dc::notice, "DBusMatchSignal::match_callback()");
  m_match_callback(message);
  signal(2);
}

void DBusMatchSignal::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case DBusMatchSignal_start:
    {
      m_dbus_connection = m_broker->run(*m_broker_key, [this](bool success){ Dout(dc::notice, "dbus_connection finished!"); signal(1); });
      Dout(dc::notice, "Requested name = \"" << m_dbus_connection->service_name() << "\".");
      set_state(DBusMatchSignal_create_message);
      wait(1);
      break;
    }
    case DBusMatchSignal_create_message:
    {
      Dout(dc::notice, "Unique name = \"" << m_dbus_connection->get_unique_name() << "\".");
      int res = sd_bus_match_signal_async(m_dbus_connection->get_bus(), nullptr,
          m_destination->service_name(), m_destination->object_path(), m_destination->interface_name(), m_destination->method_name(),
          &DBusMatchSignal::match_callback, nullptr, this);
      if (res < 0)
        THROW_ALERTC(-res, "sd_bus_match_signal_async");
      set_state(DBusMatchSignal_done);
      wait(2);
      break;
    }
    case DBusMatchSignal_done:
      finish();
      break;
  }
}

} // namespace task

