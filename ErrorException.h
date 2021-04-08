#pragma once

#include "Error.h"
#include <exception>

namespace dbus {

class ErrorException : std::exception
{
 public:
  ErrorException(ErrorConst const& error);
};

} // namespace dbus
