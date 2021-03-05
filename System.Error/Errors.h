#pragma once

#include "../ErrorCategory.h"
#include <string>
#include <iosfwd>

namespace dbus::errors {
namespace System::Error {

// All errno names.

enum Errors
{
#include "SystemErrors.h"
};

struct ErrorDomain : public ErrorDomainBase
{
  std::error_code get_error_code(std::string const& member_name) const override;
};

extern ErrorCategory<ErrorDomain, Errors> theErrorCategory;

// Functions that will be found using Argument-dependent lookup.
std::string to_string(Errors error);
std::ostream& operator<<(std::ostream& os, Errors error);
inline char const* get_domain(Errors) { return "DBus:System.Error"; }
inline std::error_code make_error_code(Errors ec) { return {ec, theErrorCategory}; }

} // namespace System::Error
} // namespace dbus::errors

// Register Errors as valid error code.
namespace std {
template<> struct is_error_code_enum<dbus::errors::System::Error::Errors> : true_type { };
} // namespace std
