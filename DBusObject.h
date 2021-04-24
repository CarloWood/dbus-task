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
    DBusObject_add_object,
    DBusObject_done
  };

 public:
  /// One beyond the largest state of this task.
  static state_type constexpr state_end = DBusObject_done + 1;

  DBusObject(CWDEBUG_ONLY(bool debug = false)) CWDEBUG_ONLY(: AIStatefulTask(debug))
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
    signal(2);
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
