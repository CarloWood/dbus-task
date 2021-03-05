#include "sys.h"
#include "Errors.h"
#include <magic_enum.hpp>
#include <iostream>

namespace dbus::errors {
namespace org::freedesktop::DBus::Error {

std::string to_string(Errors error)
{
  return std::string{magic_enum::enum_name(error)};
}

std::ostream& operator<<(std::ostream& os, Errors error)
{
  os << to_string(error);
  return os;
}

std::error_code ErrorDomain::get_error_code(std::string const& member_name) const
{
  return make_error_code(*magic_enum::enum_cast<Errors>(member_name));
}

// Instantiation of the error category object.
ErrorCategory<ErrorDomain, Errors> theErrorCategory;

} // namespace org::freedestop::DBus::Error
} // namespace dbus::errors
