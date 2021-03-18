#pragma once

namespace dbus {

class Destination
{
 private:
  char const* m_service_name;
  char const* m_object_path;
  char const* m_interface_name;
  char const* m_method_name;

 public:
  Destination(char const* service_name, char const* object_path, char const* interface_name, char const* method_name) :
    m_service_name(service_name), m_object_path(object_path), m_interface_name(interface_name), m_method_name(method_name) { }

  char const* service_name() const { return m_service_name; }
  char const* object_path() const { return m_object_path; }
  char const* interface_name() const { return m_interface_name; }
  char const* method_name() const { return m_method_name; }
};

} // namespace dbus
