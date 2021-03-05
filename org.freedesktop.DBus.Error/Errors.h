#pragma once

#include "../ErrorCategory.h"
#include <string>
#include <iosfwd>

namespace dbus::errors {
namespace org::freedesktop::DBus::Error {

// Standard errors defined for org.freedesktop.DBus.Error.*
enum Errors
{
  Failed = 1,
  NoMemory,
  ServiceUnknown,
  NameHasNoOwner,
  NoReply,
  IOError,
  BadAddress,
  NotSupported,
  LimitsExceeded,
  AccessDenied,
  AuthFailed,
  InteractiveAuthorizationRequired,
  NoServer,
  Timeout,
  NoNetwork,
  AddressInUse,
  Disconnected,
  InvalidArgs,
  FileNotFound,
  FileExists,
  UnknownMethod,
  UnknownObject,
  UnknownInterface,
  UnknownProperty,
  PropertyReadOnly,
  UnixProcessIdUnknown,
  InvalidSignature,
  InconsistentMessage,
  TimedOut,
  MatchRuleInvalid,
  InvalidFileContent,
  MatchRuleNotFound,
  SELinuxSecurityContextUnknown,
  ObjectPathInUse
};

struct ErrorDomain : public ErrorDomainBase
{
  std::error_code get_error_code(std::string const& member_name) const override;
};

extern ErrorCategory<ErrorDomain, Errors> theErrorCategory;

// Functions that will be found using Argument-dependent lookup.
std::string to_string(Errors error);
std::ostream& operator<<(std::ostream& os, Errors error);
inline char const* get_domain(Errors) { return "DBus:org.freedesktop.DBus.Error"; }
inline std::error_code make_error_code(Errors ec) { return {ec, theErrorCategory}; }

} // namespace org::freedesktop::DBus::Error
} // namespace dbus::errors

// Register Errors as valid error code.
namespace std {
template<> struct is_error_code_enum<dbus::errors::org::freedesktop::DBus::Error::Errors> : true_type { };
} // namespace std
