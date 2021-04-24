#include "sys.h"
#include "DBusMatchSignal.h"
#include "systemd_sd-bus.h"

namespace utils { using namespace threading; }

namespace task {

char const* DBusMatchSignal::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(DBusMatchSignal_start);
    AI_CASE_RETURN(DBusMatchSignal_lock1);
    AI_CASE_RETURN(DBusMatchSignal_locked1);
    AI_CASE_RETURN(DBusMatchSignal_done);
  }
  AI_NEVER_REACHED;
}

void DBusMatchSignal::match_callback(dbus::MessageRead const& message)
{
  DoutEntering(dc::notice, "DBusMatchSignal::match_callback()");
  m_match_callback(message);
  // Make sure DBusMatchSignal::match_callback is not called again.
  sd_bus_slot_unref(m_slot);
  m_slot = nullptr;
  // Unlock the connection before waking up the task.
  m_dbus_connection->unlock();
  signal(2);
}

void DBusMatchSignal::initialize_impl()
{
  DoutEntering(dc::statefultask(mSMDebug), "DBusMatchSignal::initialize_impl() [" << (void*)this << "]");
  m_slot = nullptr;
  set_state(DBusMatchSignal_start);
}

void DBusMatchSignal::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case DBusMatchSignal_start:
    {
      m_dbus_connection = m_broker->run(m_broker_key, [this](bool success){ Dout(dc::notice, "dbus_connection finished!"); signal(connection_set_up); });
      Dout(dc::notice, "Requested name = \"" << m_dbus_connection->service_name() << "\".");
      set_state(DBusMatchSignal_lock1);
      wait(connection_set_up);
      break;
    }
    case DBusMatchSignal_lock1:
      set_state(DBusMatchSignal_locked1);
      // Attempt to obtain the lock on the connection.
      if (!m_dbus_connection->lock(this, connection_locked))
      {
        wait(connection_locked);
        break;
      }
      [[fallthrough]];
    case DBusMatchSignal_locked1:
    {
      std::unique_lock<DBusMutex> lk(...);
      Dout(dc::notice, "Unique name = \"" << m_dbus_connection->get_unique_name() << "\".");
      int res = sd_bus_match_signal_async(m_dbus_connection->get_bus(), &m_slot,
          m_destination->service_name(), m_destination->object_path(), m_destination->interface_name(), m_destination->method_name(),
          &DBusMatchSignal::match_callback, nullptr, this);
      lk.unlock();
      if (res < 0)
        THROW_ALERTC(-res, "sd_bus_match_signal_async");
      set_state(DBusMatchSignal_done);
      // Wait for a call back.
      wait(2);
      break;
    }
    case DBusMatchSignal_done:
      finish();
      break;
  }
}

void DBusMatchSignal::abort_impl()
{
  if (m_slot)
  {
    DBusMutex m{m_dbus_connection};
    std::unique_lock<DBusMutex> lk(m);
    // Make sure DBusMatchSignal::s_*_callback is no longer called.
    sd_bus_slot_unref(m_slot);
    m_slot = nullptr;
  }
}

} // namespace task

