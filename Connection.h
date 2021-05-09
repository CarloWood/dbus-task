#pragma once

#include "evio/RawInputDevice.h"
#include "evio/RawOutputDevice.h"
#include "systemd_sd-bus.h"
#include "debug.h"
#ifdef CWDEBUG
#include "cwds/tracked_intrusive_ptr.h"
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
  bool m_unlocked_in_callback;                                  // Set to true when m_mutex was unlocked while inside sd_bus_process.

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
    DoutEntering(dc::notice, "Connection::connect_system()");
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
    Dout(dc::notice, "unset_unlocked_in_callback() [" << this << "]");
    m_unlocked_in_callback = false;
  }

  void set_unlocked_in_callback()
  {
    Dout(dc::notice, "set_unlocked_in_callback() [" << this << "]");
    ASSERT(!m_unlocked_in_callback);
    m_unlocked_in_callback = true;
  }

  bool is_unlocked_in_callback() const
  {
    return m_unlocked_in_callback;
  }

  HandleIOResult handle_dbus_io();

  void print_tracker_info_on(std::ostream& os)
  {
    os << this << ": " << get_unique_name();
  }

 protected:
  virtual void read_from_fd(int& allow_deletion_count, int fd);
  virtual void write_to_fd(int& allow_deletion_count, int fd);
  virtual void hup(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd)) { DoutEntering(dc::notice, "Connection::hup"); }
  virtual void err(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd)) { DoutEntering(dc::notice, "Connection::err"); close(); }
};

} // namespace dbus

#ifdef CWDEBUG
// We are tracking boost::intrusive_ptr<dbus::Connection>.
DECLARE_TRACKED_BOOST_INTRUSIVE_PTR(dbus::Connection)
#endif
