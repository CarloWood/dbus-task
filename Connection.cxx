#include "sys.h"
#include "dbus-task/Connection.h"
#include "dbus-task/DBusConnection.h"
#include <poll.h>

namespace dbus {

Connection::HandleIOResult Connection::handle_dbus_io()
{
  DoutEntering(dc::notice|continued_cf, "Connection::handle_dbus_io() = ");
#ifdef CWDEBUG
  ASSERT(m_magic == 0x12345678abcdef99);
#endif
  int ret;
  do
  {
    ret = sd_bus_process(m_bus, nullptr);
    ASSERT(m_magic == 0x12345678abcdef99);
    if (ret < 0)
    {
      THROW_ALERTC(-ret, "sd_bus_process");
    }
    if (ret && m_unlocked_in_callback)
    {
      Dout(dc::finish, "needs_relock");
      return needs_relock;
    }
  }
  while (ret);
  int flags = sd_bus_get_events(m_bus);

  // If POLLOUT is set, reset POLLIN.
  flags &= ~((flags & POLLOUT) ? POLLIN : 0);

  if ((flags & POLLOUT))
    start_output_device();
  else if ((flags & POLLIN))
    start_input_device();

  Dout(dc::finish, (m_unlocked_in_callback ? "unlocked_and_io_handled" : "io_handled"));
  return m_unlocked_in_callback ? unlocked_and_io_handled : io_handled;
}

void Connection::read_from_fd(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd))
{
  DoutEntering(dc::notice, "Connection::read_from_fd()");
  // We should not stop the input device because that can be interpreted as that we're
  // done with the socket, leading to, for example, program termination.
  //
  // Wake up the DBusConnection task - also if it is waiting for request_name_callback.
  if (!m_handle_io->signal(task::DBusHandleIO::have_dbus_io))
    // If signal returned false then the task did not wake up, possibly
    // because it is halted due to a full threadpool queue. In that case
    // stop monitoring the fd for input.
    stop_input_device();
}

void Connection::write_to_fd(int& UNUSED_ARG(allow_deletion_count), int UNUSED_ARG(fd))
{
  DoutEntering(dc::notice, "Connection::write_to_fd()");
  // Although task::DBusHandleIO uses a direct handler, it is possible that it is already
  // running. In that case the signal will be ignored (and handled by the thread that
  // is already running it a bit later). Therefore we must stop the output device or
  // the EventThread will keep calling this function over and over, thinking that
  // more has to be written.
  stop_output_device();
  // Wake up the DBusConnection task - also if it is waiting for request_name_callback.
  m_handle_io->signal(task::DBusHandleIO::have_dbus_io);
}

} // namespace dbus
