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
  boost::intrusive_ptr<task::Broker<task::DBusConnection>> m_broker;
  dbus::DBusConnectionBrokerKey const* m_broker_key;
  dbus::Destination const* m_destination;
  std::function<void(dbus::MessageRead const&)> m_match_callback;
  boost::intrusive_ptr<task::DBusConnection const> m_dbus_connection;

 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  /// The different states of the stateful task.
  enum DBusMatchSignal_state_type {
    DBusMatchSignal_start = direct_base_type::state_end,
    DBusMatchSignal_create_message,
    DBusMatchSignal_done
  };

 public:
  /// One beyond the largest state of this task.
  static state_type constexpr state_end = DBusMatchSignal_done + 1;

  DBusMatchSignal(CWDEBUG_ONLY(bool debug = false)) CWDEBUG_ONLY(: AIStatefulTask(debug))
  {
    DoutEntering(dc::statefultask(mSMDebug), "DBusMatchSignal() [" << (void*)this << "]");
  }

  // The Destination object must have a life-time longer than the time it takes to finish task::DBusConnection.
  void set_destination(boost::intrusive_ptr<task::Broker<task::DBusConnection>> broker, dbus::DBusConnectionBrokerKey const* broker_key, dbus::Destination const* destination)
  {
    m_broker = broker;
    m_broker_key = broker_key;
    m_destination = destination;
  }

  void set_match_callback(std::function<void(dbus::MessageRead const&)> match_callback)
  {
    m_match_callback = std::move(match_callback);
  }

 private:
  void match_callback(dbus::MessageRead const& message);

  static int match_callback(sd_bus_message* m, void* userdata, sd_bus_error* UNUSED_ARG(empty_error))
  {
    static_cast<DBusMatchSignal*>(userdata)->match_callback(m);
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

  /// Handle mRunState.
  void multiplex_impl(state_type run_state) override;
};

} //namespace task
