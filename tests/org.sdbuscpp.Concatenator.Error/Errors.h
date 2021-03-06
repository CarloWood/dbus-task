#pragma once

#include "dbus-task/ErrorCategory.h"
#include <string>
#include <iosfwd>

namespace dbus::errors {
namespace org::sdbuscpp::Concatenator::Error {

// There is only one error.
enum Errors
{
  NoNumbers = 1,
};

struct ErrorDomain : public ErrorDomainBase
{
  std::error_code get_error_code(std::string const& member_name) const override;
};

extern ErrorCategory<ErrorDomain, Errors> theErrorCategory;

// Functions that will be found using Argument-dependent lookup.
std::string to_string(Errors error);
std::ostream& operator<<(std::ostream& os, Errors error);
inline char const* get_domain(Errors) { return "DBus:org.sdbuscpp.Concatenator.Error"; }
inline std::error_code make_error_code(Errors ec) { return {ec, theErrorCategory}; }

} // namespace org::sdbuscpp::Concatenator::Error
} // namespace dbus::errors

// Register Errors as valid error code.
namespace std {
template<> struct is_error_code_enum<dbus::errors::org::sdbuscpp::Concatenator::Error::Errors> : true_type { };
} // namespace std
