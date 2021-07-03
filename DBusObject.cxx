#include "sys.h"
#include "systemd_sd-bus.h"
#include "DBusObject.h"

namespace task {

char const* DBusObject::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(DBusObject_start);
    AI_CASE_RETURN(DBusObject_wait_for_lock);
    AI_CASE_RETURN(DBusObject_locked);
    AI_CASE_RETURN(DBusObject_done);
  }
  AI_NEVER_REACHED;
}

void DBusObject::initialize_impl()
{
  DoutEntering(dc::statefultask(mSMDebug), "DBusObject::initialize_impl() [" << this << "]");
  m_slot = nullptr;
  set_state(DBusObject_start);
}

void DBusObject::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case DBusObject_start:
    {
      m_dbus_connection = m_broker->run(*m_broker_key, [this](bool success){ Dout(dc::statefultask(mSMDebug), "dbus_connection finished!"); signal(connection_set_up); });
      Dout(dc::dbus, "Requested name = \"" << m_dbus_connection->service_name() << "\" [" << this << "]");
      set_state(DBusObject_wait_for_lock);
      wait(connection_set_up);
      break;
    }
    case DBusObject_wait_for_lock:
      set_state(DBusObject_locked);
      // Attempt to obtain the lock on the connection.
      if (!m_dbus_connection->lock(this, connection_locked))
      {
        wait(connection_locked);
        break;
      }
      [[fallthrough]];
    case DBusObject_locked:
    {
      set_state(DBusObject_done);
      DBusLock lock(m_dbus_connection);
      Dout(dc::dbus, "Unique name = \"" << m_dbus_connection->get_unique_name() << "\" [" << this << "]");
      int res = sd_bus_add_object(m_dbus_connection->get_bus(), &m_slot, m_interface->object_path(), &DBusObject::s_object_callback, this);
      lock.unlock();
      if (res < 0)
        THROW_ALERTC(-res, "sd_bus_add_object");
      wait(stop_called);
      break;
    }
    case DBusObject_done:
      finish();
      break;
  }
}

void DBusObject::abort_impl()
{
  DoutEntering(dc::statefultask(mSMDebug), "DBusObject::abort_impl() [" << this << "]");
  if (m_slot)
  {
    m_dbus_connection->lock_blocking(this);
    DBusLock lock(m_dbus_connection);
    // Make sure DBusObject::s_*_callback is no longer called.
    Dout(dc::statefultask(mSMDebug), "Calling sd_bus_slot_unref(m_slot) and setting m_slot to nullptr (making sure the callback is no longer called)");
    sd_bus_slot_unref(m_slot);
    m_slot = nullptr;
  }
}

} // namespace task

