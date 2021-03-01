#include "sys.h"
#include "Error.h"
#include <iostream>

namespace dbus_task {

void print_ErrorConst_on(std::ostream& os, ErrorConst const& error)
{
  os << "{ ";
  if (error.m_error.name)
  {
    os << '"' << error.m_error.name << "\" ";
    if (error.m_error.message)
      os << '[' << error.m_error.message << ']';
  }
  os << '}';
}

} // namespace dbus_task
