#pragma once

#include "Interface.h"
#include "DBusHandleIO.h"
#include "statefultask/AIStatefulTask.h"
#include "debug.h"
#include <iomanip>

namespace task {

class DBusConnection;

// Initialization data (before running the task).
class DBusConnectionData
{
 protected:
  // Input variables.
  std::string m_service_name;                                   // Requested "well known" service name, if any (if this is a service).
  int m_flags;                                                  // Flags specifying how to handle duplicated service name requests.
  bool m_use_system_bus;                                        // Use system bus if true, user bus otherwise.

  // Set m_flags to zero because when request_service_name is
  // not called then it is still used to calculate a hash.
  DBusConnectionData() : m_flags(0), m_use_system_bus(false) { }

  // Used by DBusConnectionBrokerKey.
  void initialize(DBusConnection& dbus_connection) const;

  bool operator==(DBusConnectionData const& other) const
  {
    return m_service_name == other.m_service_name && m_flags == other.m_flags && m_use_system_bus == other.m_use_system_bus;
  }

  void print_on(std::ostream& os) const
  {
    os << '{';
    if (!m_service_name.empty())
    {
      os << "service_name:\"" << m_service_name << "\", flags:";
      if (!m_flags)
        os << '0';
      else
      {
        char const* prefix = "";
        if ((m_flags & SD_BUS_NAME_ALLOW_REPLACEMENT))
        {
          os << prefix << "SD_BUS_NAME_ALLOW_REPLACEMENT";
          prefix = "|";
        }
        if ((m_flags & SD_BUS_NAME_REPLACE_EXISTING))
        {
          os << prefix << "SD_BUS_NAME_REPLACE_EXISTING";
          prefix = "|";
        }
        if ((m_flags & SD_BUS_NAME_QUEUE))
        {
          os << prefix << "SD_BUS_NAME_QUEUE";
          prefix = "|";
        }

      }
      os << ", use_system_bus:" << std::boolalpha << m_use_system_bus;
    }
    os << '}';
  }

 public:
  /// Request a service name for this connection.
  //
  // flags is the bit-wise OR of zero or more of
  //
  //   SD_BUS_NAME_ALLOW_REPLACEMENT
  //       After acquiring the name successfully, permit other peers to take over the name when they try to acquire
  //       it with the SD_BUS_NAME_REPLACE_EXISTING flag set. If SD_BUS_NAME_ALLOW_REPLACEMENT is not set on the
  //       original request, such a request by other peers will be denied.
  //
  //   SD_BUS_NAME_REPLACE_EXISTING
  //       Take over the name if it was already acquired by another peer, and that other peer has permitted takeover
  //       by setting SD_BUS_NAME_ALLOW_REPLACEMENT while acquiring it.
  //
  //   SD_BUS_NAME_QUEUE
  //       Queue the acquisition of the name when the name is already taken.
  //
  void request_service_name(std::string service_name, int flags = 0) { m_service_name = std::move(service_name); m_flags = flags; }

  /// Set if this connection should be to the system bus or the user bus.
  void set_use_system_bus(bool use_system_bus) { m_use_system_bus = use_system_bus; }
};

class DBusConnection : public AIStatefulTask, public DBusConnectionData
{
 public:
  static constexpr condition_type request_name_callback = 1;
  static constexpr condition_type connection_locked = 2;

 private:
  // Wrapped data.
  boost::intrusive_ptr<DBusHandleIO> m_handle_io;               // Pointer to the task containing the statefultask mutex.

  // Internal usage:
  sd_bus_slot* m_slot;                                          // To cancel a connection upon abort.
  sd_bus_message* m_request_name_async_callback_message;        // To tranfer service name request reply message from request_name_async_callback
                                                                // to state machine (DBusConnection_wait_for_request_name_result).
 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  /// The different states of the stateful task.
  enum DBusConnection_state_type {
    DBusConnection_start = direct_base_type::state_end,
    DBusConnection_done,
    DBusConnection_request_name_async_wait_for_lock,
    DBusConnection_request_name_async,
    DBusConnection_wait_for_request_name_result_wait_for_lock,
    DBusConnection_wait_for_request_name_result
  };

 public:
  /// One beyond the largest state of this task.
  static constexpr state_type state_end = DBusConnection_wait_for_request_name_result + 1;

  /// Construct a DBusConnection object.
  DBusConnection(CWDEBUG_ONLY(bool debug = false)) : AIStatefulTask(CWDEBUG_ONLY(debug))
  {
    DoutEntering(dc::statefultask(mSMDebug), "DBusConnection() [" << (void*)this << "]");
  }

  /// Return the service name that was requested with request_service_name.
  std::string const& service_name() const { return m_service_name; }

  void lock_blocking(AIStatefulTask* task) const
  {
    m_handle_io->lock_blocking(task);
  }

  bool lock(AIStatefulTask* task, condition_type condition) const
  {
    return m_handle_io->lock(task, condition);
  }

  void unlock() const
  {
    m_handle_io->unlock(false);
  }

  AIStatefulTaskMutex& mutex() const
  {
    return m_handle_io->mutex();
  }

  /// Return the unique name of this connection.
  std::string get_unique_name() const
  {
    // The task must be successfully finished before you can call this function.
    ASSERT(finished() && !aborted());
    return m_handle_io->connection()->get_unique_name();
  }

  /// Return the underlaying sd_bus* of this connection.
  /// Only valid after the task successfully finished.
  sd_bus* get_bus() const
  {
    // The task must be successfully finished before you call this function.
    ASSERT(finished() && !aborted());
    return m_handle_io->connection()->get_bus();
  }

  void terminate()
  {
    DoutEntering(dc::statefultask(mSMDebug), "DBusConnection::terminate()");
    m_handle_io->abort();
  }

 protected:
  /// Call finish() (or abort()), not delete.
  ~DBusConnection() override
  {
    DoutEntering(dc::statefultask(mSMDebug), "~DBusConnection() [" << (void*)this << "]");
    terminate();
  }

  void obtained_lock() const
  {
    m_handle_io->obtained_lock();
  }

  // Implementation of virtual functions of AIStatefulTask.
  char const* condition_str_impl(condition_type condition) const override;
  char const* state_str_impl(state_type run_state) const override;
  char const* task_name_impl() const override;
  void initialize_impl() override;
  void multiplex_impl(state_type run_state) override;
  void finish_impl() override;
  void abort_impl() override;

 private:
  // This is the callback for sd_bus_request_name_async.
  static int s_request_name_async_callback(sd_bus_message* m, void* userdata, sd_bus_error* ret_error);
  int request_name_async_callback(sd_bus_message* m);
};

class DBusLock : public statefultask::AdoptLock
{
 public:
  DBusLock(boost::intrusive_ptr<DBusConnection const> const& connection) : statefultask::AdoptLock(connection->mutex()) { }
};

} // namespace task
