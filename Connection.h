#include "sys.h"
#include "evio/inet_support.h"
#include "evio/SocketAddress.h"
#include "evio/RawInputDevice.h"
#include "evio/RawOutputDevice.h"
#include "utils/AIAlert.h"
#include "utils/at_scope_end.h"
#include <systemd/sd-bus.h>
#include <memory>
#include <string_view>
#include <cstdlib>      // secure_getenv
#include <cstdio>       // asprintf
#include <cstring>      // strlen, strchr
#include "debug.h"

namespace dbus_task {

class Connection : public evio::RawInputDevice, public evio::RawOutputDevice
{
 private:
  sd_bus* m_bus;

 public:
  Connection()
  {
    DoutEntering(dc::notice, "Connection::Connection()");
  }

#if 0
  void connected_cb(int&, bool)
  {
    DoutEntering(dc::notice, "Connection::connected_cb()");
  }
#endif

  void connect()
  {
    DoutEntering(dc::notice, "Connection::create_connection()");
#if 0
    int fd = create_tcp_connection(sdbus::decode_bus_address(sdbus::get_bus_address_user()), 8192, 8192, 0, 0, SocketAddress{});
    if (fd == -1)
      THROW_ALERTE("Failed to create D-Bus connection to \"[ADDRESS]\"", AIArgs("[ADDRESS]", sdbus::get_bus_address_user()));
    m_connection = sdbus::createSessionBusConnection(fd, fd);
#endif
    std::string description = "Connection";
    int ret = sd_bus_open_user_with_description(&m_bus, description.c_str());           // Create and connects a socket and writes the initial messages to it.
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_open_user_with_description");
    int fd = ret = sd_bus_get_fd(m_bus);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_get_fd");
    fd_init(fd);
    start_output_device();
//    start_input_device();                                                               // Wait for the reply from the server.
  }

  sd_bus* get_bus() { return m_bus; }

 private:
  std::atomic_int m_inside_handle_dbus_io;

  void handle_dbus_io(int current_flags);

 protected:
  virtual void read_from_fd(int& allow_deletion_count, int fd);

  virtual void write_to_fd(int& allow_deletion_count, int fd);

  virtual void hup(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd))
  {
    DoutEntering(dc::notice, "Connection::hup");
  }

  virtual void err(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd))
  {
    DoutEntering(dc::notice, "Connection::err");
    close();
  }
};

} // namespace dbus_task
