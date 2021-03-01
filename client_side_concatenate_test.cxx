#include "sys.h"
#include "dbus-task/Connection.h"
#include "statefultask/AIStatefulTask.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "threadpool/AIThreadPool.h"
#include "evio/EventLoop.h"
#include "resolver-task/DnsResolver.h"
#include "threadsafe/Gate.h"
#include "utils/debug_ostream_operators.h"
#include "debug.h"

#include <systemd/sd-bus.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

std::ostream& operator<<(std::ostream& os, sd_bus_message* message)
{
  char const* destination = sd_bus_message_get_destination(message);
  char const* path = sd_bus_message_get_path(message);
  char const* interface = sd_bus_message_get_interface(message);
  char const* member = sd_bus_message_get_member(message);
  os << '{';
  os << "destination:\"" << (destination ? destination : "nullptr") << "\", ";
  os << "path:\"" << (path ? path : "nullptr") << "\", ";
  os << "interface:\"" << (interface ? interface : "nullptr") << "\", ";
  os << "member:\"" << (member ? member : "nullptr") << "\"";
  return os << '}';
}

std::ostream& operator<<(std::ostream& os, sd_bus_error const* error)
{
  if (!error)
    os << "nullptr";
  else
  {
    int error_number = sd_bus_error_get_errno(error);
    os << '{';
    os << "name:\"" << (error->name ? error->name : "nullptr") << "\", ";
    os << "message:\"" << (error->message ? error->message : "nullptr") << "\", ";
    os << "errno:" << error_number << " [" << std::strerror(error_number) << "]";
    os << '}';
  }
  return os;
}

int onConcatenated(sd_bus_message* message, void* userdata, sd_bus_error* error)
{
  DoutEntering(dc::notice, "onConcatenated(" << message << ", " << userdata << ", " << error << ")");

  char const* str;
  int ret = sd_bus_message_read(message, "s", &str);
  if (ret < 0)
    THROW_ALERTC(-ret, "sd_bus_message_read");
  std::string concatenatedString = str;

  Dout(dc::notice, "Received signal with concatenated string " << concatenatedString);

  return 0;
}

aithreadsafe::Gate gate;

int callback(sd_bus_message* m, void* userdata, sd_bus_error* UNUSED_ARG(empty_error))
{
  DoutEntering(dc::notice, "callback(" << m << ", " << userdata << ", emtpy_error)");
  int is_error = sd_bus_message_is_method_error(m, nullptr);
  if (is_error < 0)
    THROW_ALERTC(-is_error, "sd_bus_message_is_method_error");
  if (is_error)
  {
    sd_bus_error const* error = sd_bus_message_get_error(m);
    ASSERT(error);
    Dout(dc::notice, "Message is an error: " << error);
    gate.open();
  }
  else
  {
    char const* str;
    int ret = sd_bus_message_read(m, "s", &str);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_message_read");
    std::string result = str;
    Dout(dc::notice, "result = \"" << result << "\".");
    assert(result == "1:2:3");
  }

  return 0;
}

int main(int argc, char* argv[])
{
  Debug(debug::init());
  Dout(dc::notice, "Entering main()");

  // Create a AIMemoryPagePool object (must be created before thread_pool).
  [[maybe_unused]] AIMemoryPagePool mpp;

  // Set up the thread pool for the application.
  int const number_of_threads = 8;                      // Use a thread pool of 8 threads.
  int const max_number_of_threads = 16;                 // This can later dynamically increased to 16 if needed.
  int const queue_capacity = number_of_threads;
  int const reserved_threads = 1;                       // Reserve 1 thread for each priority.
  // Create the thread pool.
  AIThreadPool thread_pool(number_of_threads, max_number_of_threads);
  // And the thread pool queues.
  [[maybe_unused]] AIQueueHandle high_priority_queue   = thread_pool.new_queue(queue_capacity, reserved_threads);
  [[maybe_unused]] AIQueueHandle medium_priority_queue = thread_pool.new_queue(queue_capacity, reserved_threads);
                   AIQueueHandle low_priority_queue    = thread_pool.new_queue(queue_capacity);

  // Main application begin.
  try
  {
    // Set up the I/O event loop.
    evio::EventLoop event_loop(low_priority_queue);
    resolver::Scope resolver_scope(low_priority_queue, false);

    // Create D-Bus connection to the system bus and requests name on it.
  //  const char* serviceName = "org.sdbuscpp.concatenator";
  //  auto connection         = sdbus::createSystemBusConnection(serviceName);
    auto connection = evio::create<dbus_task::Connection>();
    connection->connect();

    // Create proxy object for the concatenator object on the server side. Since here
    // we are creating the proxy instance without passing connection to it, the proxy
    // will create its own connection automatically, and it will be system bus connection.
    char const* destinationName = "org.sdbuscpp.concatenator";
    char const* objectPath      = "/org/sdbuscpp/concatenator";
//    auto concatenatorProxy      = sdbus::createProxy(connection->get_connection(), destinationName, objectPath);

    // Let's subscribe for the 'concatenated' signals
    char const* interfaceName = "org.sdbuscpp.Concatenator";
//    concatenatorProxy->registerSignalHandler(interfaceName, "concatenated", &onConcatenated);
//    concatenatorProxy->finishRegistration();

    sd_bus_match_signal_async(connection->get_bus(), nullptr, destinationName, objectPath, interfaceName, "concatenated", onConcatenated, nullptr, nullptr);

    std::vector<int32_t> numbers = {1, 2, 3};
    std::string separator    = ":";

    // Invoke concatenate on given interface of the object
    {
      //auto method = concatenatorProxy->createMethodCall(interfaceName, "concatenate");
      sd_bus_message* message;
      int ret;
      ret = sd_bus_message_new_method_call(connection->get_bus(), &message, destinationName, objectPath, interfaceName, "concatenate");
      if (ret < 0)
        THROW_ALERTC(-ret, "sd_bus_message_new_method_call");
      ret = sd_bus_message_append_array(message, 'i', &numbers[0], sizeof(int32_t) * numbers.size());
      if (ret < 0)
        THROW_ALERTC(-ret, "sd_bus_message_append_array");
      ret = sd_bus_message_append_basic(message, 's', separator.c_str());
      if (ret < 0)
        THROW_ALERTC(-ret, "sd_bus_message_append_basic");
      ret = sd_bus_call_async(connection->get_bus(), nullptr, message, callback, nullptr, 0);
      if (ret < 0)
        THROW_ALERTC(-ret, "sd_bus_call_async");
    }

    // Invoke concatenate again, this time with no numbers and we shall get an error
    {
      //auto method = concatenatorProxy->createMethodCall(interfaceName, "concatenate");
      sd_bus_message* message;
      int ret;
      ret = sd_bus_message_new_method_call(connection->get_bus(), &message, destinationName, objectPath, interfaceName, "concatenate");
      if (ret < 0)
        THROW_ALERTC(-ret, "sd_bus_message_new_method_call");
      ret = sd_bus_message_append_array(message, 'i', &numbers[0], 0);
      if (ret < 0)
        THROW_ALERTC(-ret, "sd_bus_message_append_array");
      ret = sd_bus_message_append_basic(message, 's', separator.c_str());
      if (ret < 0)
        THROW_ALERTC(-ret, "sd_bus_message_append_basic");
      ret = sd_bus_call_async(connection->get_bus(), nullptr, message, callback, nullptr, 0);
      if (ret < 0)
        THROW_ALERTC(-ret, "sd_bus_call_async");
    }

    gate.wait();
    connection->close();

    // Application terminated cleanly.
    event_loop.join();
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error << " [caught in client_side_concatenate_test.cxx].");
  }

  Dout(dc::notice, "Leaving main()");
}
