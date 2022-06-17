#pragma once

#include "evio/RawInputDevice.h"
#include "evio/RawOutputDevice.h"
#include "systemd_sd-bus.h"
#include "debug.h"

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
  bool m_unlocked_in_callback;          // Set to true when m_mutex was unlocked while inside sd_bus_process.

#if CW_DEBUG
  uint64_t m_magic = 0x12345678abcdef99;
#endif

 public:
  Connection(task::DBusHandleIO* handle_io) : m_handle_io(handle_io) { }
  ~Connection() { DEBUG_ONLY(m_magic = 0); }

  void connect_user(std::string description = "Connection")
  {
    DoutEntering(dc::dbus, "dbus::Connection::connect_user()");
    int ret = sd_bus_open_user_with_description(&m_bus, description.c_str());           // Create and connects a socket and writes the initial messages to it.
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_open_user_with_description");
    int fd = ret = sd_bus_get_fd(m_bus);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_get_fd");
    fd_init(fd);
    // Do not start the output device yet!
    // Doing so would cause write_to_fd to be called simply because we can write,
    // but that function calls task::DBusHandleIO::signal(task::DBusHandleIO::have_dbus_io)
    // which would be lost if task::DBusHandleIO hasn't reached the corresponding
    // wait(have_dbus_io) yet.
    //
    // Once task::DBusHandleIO has reached state DBusHandleIO_start it will
    // call handle_io_ready(), which will start the output device.
  }

 private:
  friend class task::DBusHandleIO;
  void handle_io_ready()
  {
    start_output_device();
  }

 public:
  void connect_system(std::string description = "Connection")
  {
    DoutEntering(dc::dbus, "dbus::Connection::connect_system()");
    int ret = sd_bus_open_system_with_description(&m_bus, description.c_str());
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_open_system_with_description");
    int fd = ret = sd_bus_get_fd(m_bus);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_get_fd");
    fd_init(fd);
    // Do not start the output device yet! See above.
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

 public:
  enum HandleIOResult {
    needs_relock,
    io_handled,
    unlocked_and_io_handled
  };

  void unset_unlocked_in_callback()
  {
    m_unlocked_in_callback = false;
  }

  void set_unlocked_in_callback()
  {
    ASSERT(!m_unlocked_in_callback);
    m_unlocked_in_callback = true;
  }

  bool is_unlocked_in_callback() const
  {
    return m_unlocked_in_callback;
  }

  HandleIOResult handle_dbus_io();

 protected:
  void read_from_fd(int& allow_deletion_count, int fd) override;
  void write_to_fd(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd)) override;
  void hup(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd)) override { DoutEntering(dc::notice, "dbus::Connection::hup"); }
  void err(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd)) override { DoutEntering(dc::notice, "dbus::Connection::err"); close(); }
};

} // namespace dbus
