#pragma once

#include "evio/RawInputDevice.h"
#include "evio/RawOutputDevice.h"
#include "systemd_sd-bus.h"
#if 0
#include "DBusHandleIO.h"
#include "evio/inet_support.h"
#include "evio/SocketAddress.h"
#include "utils/AIAlert.h"
#include "utils/at_scope_end.h"
#include <memory>
#include <string_view>
#include <cstdlib>      // secure_getenv
#include <cstdio>       // asprintf
#include <cstring>      // strlen, strchr
#include "debug.h"
#endif

namespace task {
class DBusConnection;
class DBusHandleIO;
} // namespace task

namespace dbus {

class Connection : public evio::RawInputDevice, public evio::RawOutputDevice
{
  using condition_type = uint32_t;      // Must be the same as AIStatefulTask::condition_type

 private:
  sd_bus* m_bus;
  task::DBusHandleIO* m_handle_io;

#ifdef CWDEBUG
  uint64_t m_magic = 0x12345678abcdef99;
#endif

 public:
  Connection(task::DBusHandleIO* handle_io) : m_handle_io(handle_io)
  {
    DoutEntering(dc::notice, "Connection::Connection()");
  }

  ~Connection()
  {
    DoutEntering(dc::notice, "Connection::~Connection()");
    Debug(m_magic = 0);
  }

  void connect_user(std::string description = "Connection")
  {
    DoutEntering(dc::notice, "Connection::connect_user()");
    int ret = sd_bus_open_user_with_description(&m_bus, description.c_str());           // Create and connects a socket and writes the initial messages to it.
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_open_user_with_description");
    int fd = ret = sd_bus_get_fd(m_bus);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_get_fd");
    fd_init(fd);
    start_output_device();
  }

  void connect_system(std::string description = "Connection")
  {
    DoutEntering(dc::notice, "Connection::connect_system()");
    int ret = sd_bus_open_system_with_description(&m_bus, description.c_str());
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_open_system_with_description");
    int fd = ret = sd_bus_get_fd(m_bus);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_get_fd");
    fd_init(fd);
    start_output_device();
  }

  sd_bus* get_bus() { return m_bus; }
  std::string get_unique_name() const
  {
    char const* unique_name;
    int ret = sd_bus_get_unique_name(m_bus, &unique_name);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_get_unique_name");
    return unique_name;
  }

  void handle_dbus_io();

 protected:
  virtual void read_from_fd(int& allow_deletion_count, int fd);
  virtual void write_to_fd(int& allow_deletion_count, int fd);
  virtual void hup(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd)) { DoutEntering(dc::notice, "Connection::hup"); }
  virtual void err(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd)) { DoutEntering(dc::notice, "Connection::err"); close(); }
};

} // namespace dbus
