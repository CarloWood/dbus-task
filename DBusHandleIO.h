#pragma once

#include "Connection.h"
#include "statefultask/AIStatefulTask.h"
#include "debug.h"

namespace task {

class DBusHandleIO : public AIStatefulTask
{
 public:
  static constexpr condition_type have_dbus_io = 1;
  static constexpr condition_type connection_locked = 2;

 private:
  boost::intrusive_ptr<dbus::Connection> m_connection;          // Pointer to the Connection that is being used.
  mutable AIStatefulTaskMutex m_mutex;                          // Connection specific task mutex.

 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  /// The different states of the stateful task.
  enum DBusHandleIO_state_type {
    DBusHandleIO_start = direct_base_type::state_end,
    DBusHandleIO_wait_for_lock,
    DBusHandleIO_locked,
    DBusHandleIO_done
  };

 public:
  /// One beyond the largest state of this task.
  static constexpr state_type state_end = DBusHandleIO_done + 1;

  DBusHandleIO(CWDEBUG_ONLY(bool debug = false)) : AIStatefulTask(CWDEBUG_ONLY(debug))
  {
    DoutEntering(dc::statefultask(mSMDebug), "DBusHandleIO() [" << (void*)this << "]");
    m_connection = evio::create<dbus::Connection>(this);
  }

  void lock_blocking(AIStatefulTask* task) const
  {
    m_mutex.lock_blocking(task);
  }

  bool lock(AIStatefulTask* task, condition_type condition) const
  {
    return m_mutex.lock(task, condition);
  }

  void obtained_lock() const
  {
    m_connection->unset_unlocked_in_callback();
  }

  void unlock(bool from_callback) const
  {
    if (from_callback)
      m_connection->set_unlocked_in_callback();
    m_mutex.unlock();
  }

  boost::intrusive_ptr<dbus::Connection> const& connection() const
  {
    return m_connection;
  }

  AIStatefulTaskMutex& mutex()
  {
    return m_mutex;
  }

 protected:
  /// Call finish() (or abort()), not delete.
  ~DBusHandleIO() override
  {
    DoutEntering(dc::statefultask(mSMDebug), "~DBusHandleIO() [" << (void*)this << "]");
    if (m_connection)
      m_connection->close();
  }

  // Implementation of virtual functions of AIStatefulTask.
  char const* condition_str_impl(condition_type condition) const override;
  char const* state_str_impl(state_type run_state) const override;
  char const* task_name_impl() const override;
  void multiplex_impl(state_type run_state) override;
};

} //namespace task
