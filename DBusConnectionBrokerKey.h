#pragma once

#include "DBusConnection.h"
#include "statefultask/BrokerKey.h"
#include <farmhash.h>

namespace dbus {

class DBusConnectionBrokerKey : public statefultask::BrokerKey, public task::DBusConnectionData
{
 protected:
  uint64_t hash() const final
  {
    return util::Hash64WithSeeds(m_service_name.data(), m_service_name.length(), m_use_system_bus ? 0xa38b092ee91a871fULL : 0x9ae16a3b2f90404fULL, m_flags);
  }

  void initialize(boost::intrusive_ptr<AIStatefulTask> task) const final
  {
    task::DBusConnection& dbus_connection = static_cast<task::DBusConnection&>(*task);
    task::DBusConnectionData::initialize(dbus_connection);
  }

  unique_ptr canonical_copy() const final
  {
    return unique_ptr(new DBusConnectionBrokerKey(*this));
  }

  bool equal_to_impl(BrokerKey const& other) const final
  {
    task::DBusConnectionData const& data = static_cast<DBusConnectionBrokerKey const&>(other);
    return task::DBusConnectionData::operator==(data);
  }

  void print_on(std::ostream& os) const final
  {
    task::DBusConnectionData::print_on(os);
  }
};

} // namespace dbus
