#include "sys.h"
#include "dbus-task/Connection.h"
#include <poll.h>

namespace dbus {

void Connection::handle_dbus_io(int current_flags)
{
#ifdef CWDEBUG
  ASSERT(m_magic == 0x12345678abcdef99);
  dbus::lock(m_bus);
#endif
  int ret;
  do
  {
    ret = sd_bus_process(m_bus, nullptr);
    ASSERT(m_magic == 0x12345678abcdef99);
    if (ret < 0)
    {
#ifdef CWDEBUG
      dbus::unlock(m_bus);
#endif
      THROW_ALERTC(-ret, "sd_bus_process");
    }
  }
  while (ret);
  int flags = sd_bus_get_events(m_bus);
#ifdef CWDEBUG
  dbus::unlock(m_bus);
#endif
  // If POLLOUT is set, reset POLLIN.
  flags &= ~((flags & POLLOUT) ? POLLIN : 0);
  // Here is what we want:
  //
  // current_flags      flags           action
  // POLLIN             0               stop_input_device
  // POLLIN             POLLIN
  // POLLIN             POLLOUT         stop_input_device, start_output_device
  // POLLOUT            0               stop_output_device
  // POLLOUT            POLLIN          stop_output_device, start_input_device
  // POLLOUT            POLLOUT
  //
  if (flags == current_flags)
    return;
  if ((current_flags & POLLOUT))
  {
    stop_output_device();
    if (flags)
      start_input_device();
  }
  else if ((flags & POLLIN))
  {
    stop_input_device();
    if (flags)
      start_output_device();
  }
}

void Connection::read_from_fd(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd))
{
  DoutEntering(dc::notice, "Connection::read_from_fd()");
  handle_dbus_io(POLLIN);
}

void Connection::write_to_fd(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd))
{
  DoutEntering(dc::notice, "Connection::write_to_fd()");
  handle_dbus_io(POLLOUT);
}

} // namespace dbus
