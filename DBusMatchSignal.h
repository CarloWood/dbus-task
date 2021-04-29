#pragma once

#include "Message.h"
#include "DBusConnectionBrokerKey.h"
#include "Destination.h"
#include "statefultask/Broker.h"
#include "debug.h"

namespace task {

class DBusMatchSignal : public AIStatefulTask
{
 private:
  static constexpr condition_type connection_set_up = 1;
  static constexpr condition_type connection_locked = 2;

  dbus::DBusConnectionBrokerKey m_broker_key;
  boost::intrusive_ptr<task::Broker<task::DBusConnection>> m_broker;
  dbus::Destination const* m_destination;
  std::function<void(dbus::MessageRead const&)> m_match_callback;
  boost::intrusive_ptr<task::DBusConnection const> m_dbus_connection;
  sd_bus_slot* m_slot;

 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  /// The different states of the stateful task.
  enum DBusMatchSignal_state_type {
    DBusMatchSignal_start = direct_base_type::state_end,
    DBusMatchSignal_lock1,
    DBusMatchSignal_locked1,
    DBusMatchSignal_done
  };

 public:
  /// One beyond the largest state of this task.
  static state_type constexpr state_end = DBusMatchSignal_done + 1;

  DBusMatchSignal(boost::intrusive_ptr<task::Broker<task::DBusConnection>> broker COMMA_CWDEBUG_ONLY(bool debug = false)) :
    CWDEBUG_ONLY(AIStatefulTask(debug),) m_broker(std::move(broker))
  {
    DoutEntering(dc::statefultask(mSMDebug), "DBusMatchSignal() [" << (void*)this << "]");
  }

  // The Destination object must have a life-time longer than the time it takes to finish task::DBusConnection.
  void set_signal_match(dbus::Destination const* destination)
  {
    m_destination = destination;
  }

  void set_match_callback(std::function<void(dbus::MessageRead const&)> match_callback)
  {
    m_match_callback = std::move(match_callback);
  }

  void use_system_bus(bool use_system_bus = true)
  {
    m_broker_key.set_use_system_bus(use_system_bus);
  }

#ifdef CWDEBUG
  bool is_same_bus(sd_bus* bus) const { return m_dbus_connection->get_bus() == bus; }
#endif

 private:
  void match_callback(dbus::MessageRead const& message);

  static int match_callback(sd_bus_message* m, void* userdata, sd_bus_error* UNUSED_ARG(empty_error))
  {
    DBusMatchSignal* self = static_cast<DBusMatchSignal*>(userdata);
    self->match_callback({m, self->m_dbus_connection->get_bus()});
    return 0;
  }

 protected:
  /// Call finish() (or abort()), not delete.
  ~DBusMatchSignal() override
  {
    DoutEntering(dc::statefultask(mSMDebug), "~DBusMatchSignal() [" << (void*)this << "]");
  }

  /// Implemenation of state_str for run states.
  char const* state_str_impl(state_type run_state) const override;

  /// Run bs_initialize.
  void initialize_impl() override;

  /// Handle mRunState.
  void multiplex_impl(state_type run_state) override;

  /// Called for base state @ref bs_abort.
  void abort_impl() override;
};

} //namespace task
