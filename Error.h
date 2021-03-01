#pragma once

#include <systemd/sd-bus.h>
#include <iosfwd>
#include <string>

namespace dbus_task {

// This is a POD type, so that we can declare constants in header files.
//
// Usage:
//
// static constexpr ErrorConst foo_error = { SD_BUS_ERROR_MAKE_CONST{ "org.example.name", "Error description" } };
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
  Error(Error&& error) { sd_bus_error_move(&m_error, &error.m_error); }
  Error(int errno) { m_error = SD_BUS_ERROR_NULL; sd_bus_error_set_errno(&m_error, errno); }
  ~Error() { sd_bus_error_free(&m_error); }
  // If you want to pass a char const* string _constant_, please pass them to SD_BUS_ERROR_MAKE_CONST and use ErrorConst.
  explicit Error(std::string const& name) { sd_bus_error_set(&m_error, name.c_str(), nullptr); }
  explicit Error(std::string const& name, std::string const& message) { sd_bus_error_set(&m_error, name.c_str(), message.c_str()); }

  Error& operator=(ErrorConst const& constant) { sd_bus_error_free(&m_error); sd_bus_error_copy(&m_error, &constant.m_error); return *this; }
  Error& operator=(Error&& error) { sd_bus_error_free(&m_error); sd_bus_error_move(&m_error, &error.m_error); return *this; }

  void print_on(std::ostream& os) const { print_ErrorConst_on(os, m_error); }

  bool is_set() const { return sd_bus_error_is_set(&m_error); }
};

} // namespace dbus_task
