#pragma once

#include "Message.h"
#include "Destination.h"
#include "statefultask/AIStatefulTask.h"
#include "debug.h"

namespace task {

class DBusMethodCall : public AIStatefulTask
{
 private:
  dbus::Message m_message;

 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  /// The different states of the stateful task.
  enum DBusMethodCall_state_type {
    DBusMethodCall_start = direct_base_type::state_end,
    DBusMethodCall_wait_for_request_name_result,
    DBusMethodCall_done
  };

 public:
  /// One beyond the largest state of this task.
  static state_type constexpr state_end = DBusMethodCall_done + 1;

  DBusMethodCall(CWDEBUG_ONLY(bool debug = false)) CWDEBUG_ONLY(: AIStatefulTask(debug))
  {
    DoutEntering(dc::statefultask(mSMDebug), "DBusMethodCall() [" << (void*)this << "]");
  }

  void create_message(boost::intrusive_ptr<task::DBusConnection const> connection, dbus::Destination const& destination)
  {
    m_message.create_message(std::move(connection), destination);
  }

  template <typename InputIt>
  DBusMethodCall& append(InputIt first, InputIt last)
  {
    m_message.append(first, last);
    return *this;
  }

  template<typename T>
  DBusMethodCall& append(T basic_type)
  {
    m_message.append(basic_type);
    return *this;
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
};

} //namespace task
