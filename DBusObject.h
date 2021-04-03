#pragma once

#include "Message.h"
#include "DBusConnectionBrokerKey.h"
#include "Interface.h"
#include "statefultask/Broker.h"
#include "debug.h"

namespace task {

class DBusObject : public AIStatefulTask
{
 private:
  boost::intrusive_ptr<task::Broker<task::DBusConnection>> m_broker;
  dbus::DBusConnectionBrokerKey const* m_broker_key;
  dbus::Interface const* m_interface;
  std::function<void(dbus::MessageRead const&)> m_object_callback;
  boost::intrusive_ptr<task::DBusConnection const> m_dbus_connection;
  sd_bus_vtable const* m_vtable;
  sd_bus_slot* m_slot;
  void* m_userdata;

 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  /// The different states of the stateful task.
  enum DBusObject_state_type {
    DBusObject_start = direct_base_type::state_end,
    DBusObject_add_vtable,
    DBusObject_done
  };

 public:
  /// One beyond the largest state of this task.
  static state_type constexpr state_end = DBusObject_done + 1;

  DBusObject(CWDEBUG_ONLY(bool debug = false)) CWDEBUG_ONLY(: AIStatefulTask(debug))
  {
    DoutEntering(dc::statefultask(mSMDebug), "DBusObject() [" << (void*)this << "]");
  }

  void set_object_callback(std::function<void(dbus::MessageRead const&)> object_callback)
  {
    m_object_callback = std::move(object_callback);
  }

  void set_interface(boost::intrusive_ptr<task::Broker<task::DBusConnection>> broker, dbus::DBusConnectionBrokerKey const* broker_key, dbus::Interface const* interface)
  {
    m_broker = broker;
    m_broker_key = broker_key;
    m_interface = interface;
  }

  // The Interface object must have a life-time longer than the time it takes to finish task::DBusConnection.
  void add_vtable(sd_bus_vtable const* vtable, void* userdata)
  {
    m_vtable = vtable;
    m_userdata = userdata;
  }

 private:
  void object_callback(dbus::MessageRead const& message);

  static int object_callback(sd_bus_message* m, void* userdata, sd_bus_error* UNUSED_ARG(empty_error))
  {
    static_cast<DBusObject*>(userdata)->object_callback(m);
    return 0;
  }

 protected:
  /// Call finish() (or abort()), not delete.
  ~DBusObject() override
  {
    DoutEntering(dc::statefultask(mSMDebug), "~DBusObject() [" << (void*)this << "]");
  }

  /// Implemenation of state_str for run states.
  char const* state_str_impl(state_type run_state) const override;

  /// Run bs_initialize.
  void initialize_impl() override;

  /// Handle mRunState.
  void multiplex_impl(state_type run_state) override;

  /// Called for base state @ref bs_finish and bs_abort.
  void finish_impl() override;
};

} //namespace task
