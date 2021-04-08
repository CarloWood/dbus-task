#pragma once

#include <systemd/sd-bus.h>
#include <iosfwd>
#include <string>
#include <system_error>
#include <cstdarg>
#include "debug.h"

namespace dbus {

// This is a POD type, so that we can declare constants in header files.
//
// Usage:
//
// In .h:
//
// namespace dbus {
// static constexpr ErrorConst foo_error = { SD_BUS_ERROR_MAKE_CONST("org.example.name", "Error description") };
// } // namespace dbus
//
// This is the ONLY way ErrorConst may be created!
//
struct ErrorConst
{
  sd_bus_error m_error;
};

extern void print_ErrorConst_on(std::ostream& os, sd_bus_error const& error);
inline std::ostream& operator<<(std::ostream& os, ErrorConst const& error) { print_ErrorConst_on(os, error.m_error); return os; }

// This class is NOT const, even though it is derived from ErrorConst.
class Error : protected ErrorConst
{
 public:
  Error() { m_error = SD_BUS_ERROR_NULL; }
  Error(ErrorConst const& constant) { m_error = SD_BUS_ERROR_NULL; /* avoid EINVAL */ sd_bus_error_copy(&m_error, &constant.m_error); }
  Error(Error const& orig) { m_error = SD_BUS_ERROR_NULL; /* avoid EINVAL */ sd_bus_error_copy(&m_error, &orig.m_error); }
  Error(Error&& error) { sd_bus_error_move(&m_error, &error.m_error); }
  ~Error() { sd_bus_error_free(&m_error); }

//  Error(int error) { m_error = SD_BUS_ERROR_NULL; sd_bus_error_set_errno(&m_error, error); }
//  Error(int error, char const* format, ...) { m_error = SD_BUS_ERROR_NULL; va_list ap; va_start(ap, format); sd_bus_error_set_errnofv(&m_error, error, format, ap); va_end(ap); }
//  int get_errno() const { return sd_bus_error_get_errno(&m_error); }

  // If you want to pass a char const* string _constant_, please pass them to SD_BUS_ERROR_MAKE_CONST and use ErrorConst.
  explicit Error(std::string const& name) { m_error = SD_BUS_ERROR_NULL; sd_bus_error_set(&m_error, name.c_str(), nullptr); }
  explicit Error(std::string const& name, std::string const& message) { m_error = SD_BUS_ERROR_NULL; sd_bus_error_set(&m_error, name.c_str(), message.c_str()); }
  // Construct an Error object from a sd_bus_error*. This makes a copy.
  Error(sd_bus_error const* error) { m_error = SD_BUS_ERROR_NULL; sd_bus_error_copy(&m_error, error); }

  Error& operator=(ErrorConst const& constant) { sd_bus_error_free(&m_error); sd_bus_error_copy(&m_error, &constant.m_error); return *this; }
  Error& operator=(Error const& rhs) { sd_bus_error_free(&m_error); sd_bus_error_copy(&m_error, &rhs.m_error); return *this; }
  Error& operator=(Error&& error) { sd_bus_error_free(&m_error); sd_bus_error_move(&m_error, &error.m_error); return *this; }

  void print_on(std::ostream& os) const { print_ErrorConst_on(os, m_error); }

  // Return true if the Error wasn't default constructed, or was assigned an error afterwards.
  bool is_set() const { return sd_bus_error_is_set(&m_error); }

  // Convert Error to an error_code.
  operator std::error_code() const;

  // Back to sd_bus_error* variable.
  void copy_to(sd_bus_error* output) const { sd_bus_error_copy(output, &m_error); }
  void move_to(sd_bus_error* output) && { sd_bus_error_move(output, &m_error); }
};

inline std::ostream& operator<<(std::ostream& os, Error const& error) { error.print_on(os); return os; }

} // namespace dbus
