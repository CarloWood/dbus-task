#pragma once

#include "Interface.h"

namespace dbus {

class Destination : public Interface
{
 protected:
  char const* m_method_name;

 public:
  Destination(char const* service_name, char const* object_path, char const* interface_name, char const* method_name) :
    Interface(service_name, object_path, interface_name), m_method_name(method_name) { }

  char const* method_name() const { return m_method_name; }
};

} // namespace dbus
