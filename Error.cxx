#include "sys.h"
#include "Error.h"
#include "ErrorDomainManager.h"
#include "utils/AIAlert.h"
#include <iostream>
#include "debug.h"

namespace dbus {

void print_ErrorConst_on(std::ostream& os, sd_bus_error const& error)
{
  os << "{ ";
  if (error.name)
  {
    os << '"' << error.name << "\" ";
    if (error.message)
      os << '[' << error.message << "] ";
  }
  os << '}';
}

Error::operator std::error_code() const
{
  if (!is_set())
    return std::error_code{};

  std::string name = m_error.name;
  std::string::size_type last_period = name.rfind('.');
  std::string domain_name;
  if (last_period != std::string::npos)
    domain_name = name.substr(0, last_period);
  std::string member_name = name.substr(last_period + 1);

  return ErrorDomainManager::instance().domain(domain_name).get_error_code(member_name);
}

} // namespace dbus
