#pragma once

#include "Message.h"
#include "DBusConnectionBrokerKey.h"
#include "Destination.h"
#include "statefultask/Broker.h"
#include "debug.h"

namespace task {

class DBusMethodCall : public AIStatefulTask
{
 private:
  dbus::Message m_message;
  boost::intrusive_ptr<task::Broker<task::DBusConnection>> m_broker;
  dbus::DBusConnectionBrokerKey const* m_broker_key;
  dbus::Destination const* m_destination;
  std::function<void(dbus::Message&)> m_params_callback;
  std::function<void(dbus::MessageRead const&)> m_reply_callback;
  boost::intrusive_ptr<task::DBusConnection const> m_dbus_connection;

 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  /// The different states of the stateful task.
  enum DBusMethodCall_state_type {
    DBusMethodCall_start = direct_base_type::state_end,
    DBusMethodCall_create_message,
    DBusMethodCall_done
  };

 public:
  /// One beyond the largest state of this task.
  static state_type constexpr state_end = DBusMethodCall_done + 1;

  DBusMethodCall(CWDEBUG_ONLY(bool debug = false)) CWDEBUG_ONLY(: AIStatefulTask(debug))
  {
    DoutEntering(dc::statefultask(mSMDebug), "DBusMethodCall() [" << (void*)this << "]");
  }

  // The Destination object must have a life-time longer than the time it takes to finish task::DBusConnection.
  void set_destination(boost::intrusive_ptr<task::Broker<task::DBusConnection>> broker, dbus::DBusConnectionBrokerKey const* broker_key, dbus::Destination const* destination)
  {
    m_broker = broker;
    m_broker_key = broker_key;
    m_destination = destination;
  }

  void set_params_callback(std::function<void(dbus::Message&)> params_callback)
  {
    m_params_callback = std::move(params_callback);
  }

  void set_reply_callback(std::function<void(dbus::MessageRead const&)> reply_callback)
  {
    m_reply_callback = std::move(reply_callback);
  }

#ifdef CWDEBUG
  bool is_same_bus(sd_bus* bus) const { return m_dbus_connection->get_bus() == bus; }
#endif

 private:
  void reply_callback(dbus::MessageRead const& message);

  static int reply_callback(sd_bus_message* m, void* userdata, sd_bus_error* UNUSED_ARG(empty_error))
  {
    DBusMethodCall* self = static_cast<DBusMethodCall*>(userdata);
    self->reply_callback({m, self->m_dbus_connection->get_bus()});
    return 0;
  }

 protected:
  /// Call finish() (or abort()), not delete.
  ~DBusMethodCall() override
  {
    DoutEntering(dc::statefultask(mSMDebug), "~DBusMethodCall() [" << (void*)this << "]");
  }

  /// Implemenation of state_str for run states.
  char const* state_str_impl(state_type run_state) const override;

  /// Handle mRunState.
  void multiplex_impl(state_type run_state) override;

  /// Called for base state @ref bs_abort.
  void abort_impl() override;
};

} //namespace task
