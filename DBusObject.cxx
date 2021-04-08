#include "sys.h"
#include <systemd/sd-bus.h>
#include "DBusObject.h"

namespace task {

char const* DBusObject::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(DBusObject_start);
    AI_CASE_RETURN(DBusObject_add_object);
    AI_CASE_RETURN(DBusObject_done);
  }
  AI_NEVER_REACHED;
}

void DBusObject::initialize_impl()
{
  DoutEntering(dc::statefultask(mSMDebug), "DBusObject::initialize_impl() [" << (void*)this << "]");
  m_slot = nullptr;
  set_state(DBusObject_start);
}

void DBusObject::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case DBusObject_start:
    {
      m_dbus_connection = m_broker->run(*m_broker_key, [this](bool success){ Dout(dc::notice, "dbus_connection finished!"); signal(1); });
      Dout(dc::notice, "Requested name = \"" << m_dbus_connection->service_name() << "\".");
      set_state(DBusObject_add_object);
      wait(1);
      break;
    }
    case DBusObject_add_object:
    {
      Dout(dc::notice, "Unique name = \"" << m_dbus_connection->get_unique_name() << "\".");
      int res = sd_bus_add_object(m_dbus_connection->get_bus(), &m_slot, m_interface->object_path(), &DBusObject::s_object_callback, this);
      if (res < 0)
        THROW_ALERTC(-res, "sd_bus_add_object");
      set_state(DBusObject_done);
      wait(2);
      break;
    }
    case DBusObject_done:
      finish();
      break;
  }
}

void DBusObject::finish_impl()
{
  if (m_slot)
  {
    // Make sure DBusObject::s_*_callback is never called (in case we get here after an abort()).
    sd_bus_slot_unref(m_slot);
    m_slot = nullptr;
  }
}

} // namespace task

