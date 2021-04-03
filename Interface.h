#pragma once

namespace dbus {

class Interface
{
 protected:
  char const* m_service_name;
  char const* m_object_path;
  char const* m_interface_name;

 public:
  Interface(char const* service_name, char const* object_path, char const* interface_name) :
    m_service_name(service_name), m_object_path(object_path), m_interface_name(interface_name) { }

  char const* service_name() const { return m_service_name; }
  char const* object_path() const { return m_object_path; }
  char const* interface_name() const { return m_interface_name; }
};

} // namespace dbus
