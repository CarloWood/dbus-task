#pragma once

#include "Message.h"
#include "DBusConnectionBrokerKey.h"
#include "Interface.h"
#include "Error.h"
#include "statefultask/Broker.h"
#include "debug.h"

namespace task {

class DBusObject : public AIStatefulTask
{
 private:
  static constexpr condition_type connection_set_up = 1;
  static constexpr condition_type connection_locked = 2;
  static constexpr condition_type stop_called = 4;

  boost::intrusive_ptr<task::Broker<task::DBusConnection>> m_broker;
  dbus::DBusConnectionBrokerKey const* m_broker_key;
  dbus::Interface const* m_interface;
  boost::intrusive_ptr<task::DBusConnection const> m_dbus_connection;
  sd_bus_slot* m_slot;
  void* m_userdata;

 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  /// The different states of the stateful task.
  enum DBusObject_state_type {
    DBusObject_start = direct_base_type::state_end,
    DBusObject_wait_for_lock,
    DBusObject_locked,
    DBusObject_done
  };

 public:
  /// One beyond the largest state of this task.
  static constexpr state_type state_end = DBusObject_done + 1;

  DBusObject(CWDEBUG_ONLY(bool debug = false)) : AIStatefulTask(CWDEBUG_ONLY(debug))
  {
    DoutEntering(dc::statefultask(mSMDebug), "DBusObject() [" << (void*)this << "]");
  }

  void set_interface(boost::intrusive_ptr<task::Broker<task::DBusConnection>> broker, dbus::DBusConnectionBrokerKey const* broker_key, dbus::Interface const* interface)
  {
    m_broker = broker;
    m_broker_key = broker_key;
    m_interface = interface;
  }

  dbus::Interface const* get_interface() const
  {
    return m_interface;
  }

  // The Interface object must have a life-time longer than the time it takes to finish task::DBusConnection.
  void set_userdata(void* userdata)
  {
    m_userdata = userdata;
  }

  void stop()
  {
    signal(stop_called);
  }

#ifdef CWDEBUG
  bool is_same_bus(sd_bus* bus) const { return m_dbus_connection->get_bus() == bus; }
#endif

 private:
  virtual bool object_callback(dbus::Message message) = 0;

  static int s_object_callback(sd_bus_message* m, void* userdata, sd_bus_error* ret_error)
  {
    int handled;
    DBusObject* self = static_cast<DBusObject*>(userdata);
    try
    {
      handled = self->object_callback({m, self->m_dbus_connection->get_bus()});
      if (handled)
      {
        // Make sure DBusObject::s_*_callback is not called again.
        Dout(dc::dbus(self->mSMDebug), "object_callback returned true: calling sd_bus_slot_unref(m_slot) and setting m_slot to nullptr (making sure the callback is no longer called)");
        sd_bus_slot_unref(self->m_slot);
        self->m_slot = nullptr;
      }
    }
    catch (dbus::Error& error)
    {
      std::move(error).move_to(ret_error);
      return 0;
    }
    return handled;
  }

 protected:
  /// Call finish() (or abort()), not delete.
  ~DBusObject() override
  {
    DoutEntering(dc::statefultask(mSMDebug), "~DBusObject() [" << (void*)this << "]");
  }

  // Implementation of virtual functions of AIStatefulTask.
  char const* condition_str_impl(condition_type condition) const override;
  char const* state_str_impl(state_type run_state) const override;
  void initialize_impl() override;
  void multiplex_impl(state_type run_state) override;
  void abort_impl() override;
};

} //namespace task
