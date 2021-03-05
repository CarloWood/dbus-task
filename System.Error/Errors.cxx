#include "sys.h"
#include "Errors.h"
#include <magic_enum.hpp>
#include <iostream>

namespace dbus::errors {
namespace System::Error {

std::string to_string(Errors error)
{
  auto sv = magic_enum::enum_name(error);
  sv.remove_prefix(3);  // Remove "SE_" from the name (see generate_SystemErrors.sh).
  return std::string{sv};
}

std::ostream& operator<<(std::ostream& os, Errors error)
{
  os << to_string(error);
  return os;
}

std::error_code ErrorDomain::get_error_code(std::string const& member_name) const
{
  return make_error_code(*magic_enum::enum_cast<Errors>("SE_" + member_name));
}

// Instantiation of the error category object.
ErrorCategory<ErrorDomain, Errors> theErrorCategory;

} // namespace System::Error
} // namespace dbus::errors
