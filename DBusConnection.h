#pragma once

#include "Connection.h"
#include "statefultask/AIStatefulTask.h"
#include "debug.h"

namespace task {

class DBusConnection;

// Initialization data (before running the task).
class DBusConnectionData
{
 protected:
  // Input variables.
  std::string m_service_name;                                   // Requested "well known" service name, if any (if this is a service).
  int m_flags;                                                  // Flags specifying how to handle duplicated serive name requests.

  // Set m_flags to zero because when request_service_name is
  // not called then it is still used to calculate a hash.
  DBusConnectionData() : m_flags(0) { }
  // Used by DBusConnectionBrokerKey.
  void initialize(DBusConnection& dbus_connection) const;

  bool operator==(DBusConnectionData const& other) const
  {
    return m_service_name == other.m_service_name && m_flags == other.m_flags;
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
};

class DBusConnection : public AIStatefulTask, public DBusConnectionData
{
 private:
  // Wrapped data.
  boost::intrusive_ptr<dbus::Connection> m_connection;          // Pointer to the Connection that is being used.

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
    DBusConnection_wait_for_request_name_result,
    DBusConnection_done
  };

 public:
  /// One beyond the largest state of this task.
  static state_type constexpr state_end = DBusConnection_done + 1;

  /// Construct a DBusConnection object.
  DBusConnection(CWDEBUG_ONLY(bool debug = false)) CWDEBUG_ONLY(: AIStatefulTask(debug))
  {
    DoutEntering(dc::statefultask(mSMDebug), "DBusConnection() [" << (void*)this << "]");
  }

  /// Return the service name that was requested with request_service_name.
  std::string const& service_name() const { return m_service_name; }

  /// Return the unique name of this connection.
  std::string get_unique_name() const
  {
    // The task must be successfully finished before you can call this function.
    ASSERT(finished() && !aborted());
    return m_connection->get_unique_name();
  }

  //FIXME: remove this
  sd_bus* get_bus() const { return m_connection->get_bus(); }

 protected:
  /// Call finish() (or abort()), not delete.
  ~DBusConnection() override
  {
    DoutEntering(dc::statefultask(mSMDebug), "~DBusConnection() [" << (void*)this << "]");
    if (m_connection)
      m_connection->close();
  }

  /// Implemenation of state_str for run states.
  char const* state_str_impl(state_type run_state) const override;

  /// Run bs_initialize.
  void initialize_impl() override;

  /// Handle mRunState.
  void multiplex_impl(state_type run_state) override;

  /// Called for base state @ref bs_finish and bs_abort.
  void finish_impl() override;

 private:
  // This is the callback for sd_bus_request_name_async.
  static int s_request_name_async_callback(sd_bus_message* m, void* userdata, sd_bus_error* ret_error);
  int request_name_async_callback(sd_bus_message* m);
};

} // namespace task
