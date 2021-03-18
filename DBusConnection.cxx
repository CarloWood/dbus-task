#include "sys.h"
#include "DBusConnection.h"
#include "Error.h"

namespace task {

char const* DBusConnection::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(DBusConnection_start);
    AI_CASE_RETURN(DBusConnection_wait_for_request_name_result);
    AI_CASE_RETURN(DBusConnection_done);
  }
  ASSERT(false);
  return "UNKNOWN STATE";
}

void DBusConnection::initialize_impl()
{
  DoutEntering(dc::statefultask(mSMDebug), "DBusConnection::initialize_impl() [" << (void*)this << "]");
  m_slot = nullptr;
  m_connection = evio::create<dbus::Connection>();
  set_state(DBusConnection_start);
  // This isn't going to work. Please call GetAddrInfo::run() with a non-immediate handler,
  // for example resolver::DnsResolver::instance().get_handler();
//  ASSERT(!default_is_immediate());
}

int DBusConnection::request_name_async_callback(sd_bus_message* m)
{
  sd_bus_message_ref(m);
  m_request_name_async_callback_message = m;
  signal(1);
  return 0;
}

int DBusConnection::s_request_name_async_callback(sd_bus_message* m, void* userdata, sd_bus_error* UNUSED_ARG(ret_error))
{
  return static_cast<DBusConnection*>(userdata)->request_name_async_callback(m);
}

void DBusConnection::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case DBusConnection_start:
      // FIXME?
      // This call could be blocking if the environment variable DBUS_SESSION_BUS_ADDRESS is
      // set to point to remote connection. In that case libsystemd will resolve the host
      // name and blocking connect to the socket using ssh :/.
      m_connection->connect_user("DBusConnection");
      if (!m_service_name.empty())
      {
        set_state(DBusConnection_wait_for_request_name_result);
        sd_bus_request_name_async(m_connection->get_bus(), &m_slot, m_service_name.c_str(), m_flags, &DBusConnection::s_request_name_async_callback, this);
        wait(1);
        break;
      }
      set_state(DBusConnection_done);
      [[fallthrough]];
    case DBusConnection_done:
      finish();
      break;
    case DBusConnection_wait_for_request_name_result:
    {
      int is_error = sd_bus_message_is_method_error(m_request_name_async_callback_message, nullptr);
      if (is_error < 0)
        THROW_FALERTC(-is_error, "sd_bus_message_is_method_error");
      if (is_error)
      {
        sd_bus_error const* error = sd_bus_message_get_error(m_request_name_async_callback_message);
        dbus::Error dbus_error = error;
        sd_bus_message_unref(m_request_name_async_callback_message);
        THROW_FALERT("[ERROR]", AIArgs("[ERROR]", dbus_error));
        abort();
      }
      else
        Dout(dc::notice, "Received message: " << m_request_name_async_callback_message);
      sd_bus_message_unref(m_request_name_async_callback_message);
      set_state(DBusConnection_done);
      break;
    }
  }
}

void DBusConnection::finish_impl()
{
  if (m_slot)
  {
    sd_bus_slot_unref(m_slot);
    m_slot = nullptr;
  }
}

void DBusConnectionData::initialize(DBusConnection& dbus_connection) const
{
  if (m_service_name.empty())
    return;
  dbus_connection.request_service_name(m_service_name, m_flags);
}

} // namespace task
