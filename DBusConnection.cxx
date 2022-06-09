#include "sys.h"
#include "DBusConnection.h"
#include "Message.h"
#include "Error.h"

namespace task {

char const* DBusConnection::condition_str_impl(condition_type condition) const
{
  switch (condition)
  {
    AI_CASE_RETURN(request_name_callback);
    AI_CASE_RETURN(connection_locked);
  }
  return direct_base_type::condition_str_impl(condition);
}

char const* DBusConnection::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(DBusConnection_start);
    AI_CASE_RETURN(DBusConnection_done);
    AI_CASE_RETURN(DBusConnection_request_name_async_wait_for_lock);
    AI_CASE_RETURN(DBusConnection_request_name_async);
    AI_CASE_RETURN(DBusConnection_wait_for_request_name_result_wait_for_lock);
    AI_CASE_RETURN(DBusConnection_wait_for_request_name_result);
  }
  AI_NEVER_REACHED;
}

void DBusConnection::initialize_impl()
{
  DoutEntering(dc::statefultask(mSMDebug), "DBusConnection::initialize_impl() [" << (void*)this << "]");
  m_slot = nullptr;
  m_request_name_async_callback_message = nullptr;
  set_state(DBusConnection_start);
  // This isn't going to work. Please call GetAddrInfo::run() with a non-immediate handler,
  // for example resolver::DnsResolver::instance().get_handler();
//  ASSERT(!default_is_immediate());
}

int DBusConnection::request_name_async_callback(sd_bus_message* message)
{
  sd_bus_message_ref(message);
  m_request_name_async_callback_message = message;
  signal(request_name_callback);
  return 0;
}

int DBusConnection::s_request_name_async_callback(sd_bus_message* message, void* userdata, sd_bus_error* UNUSED_ARG(ret_error))
{
  return static_cast<DBusConnection*>(userdata)->request_name_async_callback(message);
}

// The sd_bus that is created by this task can not be used by other tasks until this tasked finished.
// Therefore it is not needed to do connection/bus locking.
void DBusConnection::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case DBusConnection_start:
      // FIXME?
      // This call could be blocking if the environment variable DBUS_SESSION_BUS_ADDRESS is
      // set to point to remote connection. In that case libsystemd will resolve the host
      // name and blocking connect to the socket using ssh :/.
      AI_REACHED_ONCE;
      m_handle_io = statefultask::create<task::DBusHandleIO>(CWDEBUG_ONLY(mSMDebug));
      if (m_use_system_bus)
        m_handle_io->connection()->connect_system("DBusConnection - system");
      else
        m_handle_io->connection()->connect_user("DBusConnection - user");
      m_handle_io->run();
      if (!m_service_name.empty())
      {
        // Now that m_handle_io is running we can't just start calling sd_bus_* functions anymore,
        // because sd_bus is single threaded :(. Therefore obtain the task mutex before continuing.
        set_state(DBusConnection_request_name_async_wait_for_lock);
        break;
      }
      set_state(DBusConnection_done);
      [[fallthrough]];
    case DBusConnection_done:
      finish();
      break;
    case DBusConnection_request_name_async_wait_for_lock:
      set_state(DBusConnection_request_name_async);
      if (!m_handle_io->lock(this, connection_locked))
      {
        wait(connection_locked);
        break;
      }
      [[fallthrough]];
    case DBusConnection_request_name_async:
    {
      statefultask::AdoptLock lock(mutex());
      set_state(DBusConnection_wait_for_request_name_result_wait_for_lock);
      sd_bus_request_name_async(m_handle_io->connection()->get_bus(), &m_slot, m_service_name.c_str(), m_flags, &DBusConnection::s_request_name_async_callback, this);
      wait(request_name_callback);
      break;
    }
    case DBusConnection_wait_for_request_name_result_wait_for_lock:
      set_state(DBusConnection_wait_for_request_name_result);
      if (!m_handle_io->lock(this, connection_locked))
      {
        wait(connection_locked);
        break;
      }
      [[fallthrough]];
    case DBusConnection_wait_for_request_name_result:
    {
      statefultask::AdoptLock lock(mutex());
      int is_error = sd_bus_message_is_method_error(m_request_name_async_callback_message, nullptr);
      if (is_error < 0)
        THROW_FALERTC(-is_error, "sd_bus_message_is_method_error");
      if (is_error)
      {
        sd_bus_error const* error = sd_bus_message_get_error(m_request_name_async_callback_message);
        dbus::Error dbus_error = error;
        sd_bus_message_unref(m_request_name_async_callback_message);
        m_request_name_async_callback_message = nullptr;
        THROW_FALERT("[ERROR]", AIArgs("[ERROR]", dbus_error));
        abort();
      }
#if 0
      else
      {
        dbus::MessageRead message(m_request_name_async_callback_message, m_handle_io->connection()->get_bus());
        std::string s;
        message >> s;
        Dout(dc::notice, "Received message: " << s);
      }
#endif
      sd_bus_message_unref(m_request_name_async_callback_message);
      m_request_name_async_callback_message = nullptr;
      set_state(DBusConnection_done);
      break;
    }
  }
}

void DBusConnection::finish_impl()
{
  if (m_slot)
  {
    // Make sure DBusConnection::s_request_name_async_callback is never called (in case we get here after an abort()).
    sd_bus_slot_unref(m_slot);
    m_slot = nullptr;
  }
}

void DBusConnection::abort_impl()
{
  if (m_request_name_async_callback_message)
  {
    sd_bus_message_unref(m_request_name_async_callback_message);
    m_request_name_async_callback_message = nullptr;
  }
  if (m_handle_io)
  {
    m_handle_io->abort();
    m_handle_io.reset();
  }
}

void DBusConnectionData::initialize(DBusConnection& dbus_connection) const
{
  if (!m_service_name.empty())
    dbus_connection.request_service_name(m_service_name, m_flags);
  dbus_connection.set_use_system_bus(m_use_system_bus);
}

} // namespace task
