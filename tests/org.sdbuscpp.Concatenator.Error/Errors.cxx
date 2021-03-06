#include "sys.h"
#include "Errors.h"
#include <magic_enum.hpp>
#include <iostream>

namespace dbus::errors {
namespace org::sdbuscpp::Concatenator::Error {

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
  auto error = magic_enum::enum_cast<Errors>(member_name);
  return error.has_value() ? make_error_code(*error) : s_unknown_error;
}

// Instantiation of the error category object.
ErrorCategory<ErrorDomain, Errors> theErrorCategory;

} // namespace org::sdbuscpp::Concatenator::Error
} // namespace dbus::errors
