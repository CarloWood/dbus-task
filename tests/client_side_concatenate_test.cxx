#include "sys.h"
#include "org.sdbuscpp.Concatenator.Error/Errors.h"
#include "dbus-task/Connection.h"
#include "dbus-task/Error.h"
#include "dbus-task/ErrorCategory.h"
#include "dbus-task/System.Error/Errors.h"
#include "dbus-task/org.freedesktop.DBus.Error/Errors.h"
#include "dbus-task/DBusConnection.h"
#include "dbus-task/DBusMatchSignal.h"
#include "dbus-task/DBusMethodCall.h"
#include "dbus-task/Message.h"
#include "dbus-task/DBusConnectionBrokerKey.h"
#include "statefultask/AIStatefulTask.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "statefultask/Broker.h"
#include "evio/EventLoop.h"
#include "resolver-task/DnsResolver.h"
#include "threadsafe/Gate.h"
#include "threadpool/AIThreadPool.h"
#include "utils/debug_ostream_operators.h"
#include "debug.h"
#ifdef CWDEBUG
#include "cwds/tracked_intrusive_ptr.h"
#endif

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

int on_signal_concatenated(sd_bus_message* message, void* userdata, sd_bus_error* error)
{
  DoutEntering(dc::notice, "on_signal_concatenated(" << message << ", " << userdata << ", " << error << ")");

  char const* str;
  int ret = sd_bus_message_read(message, "s", &str);
  if (ret < 0)
    THROW_ALERTC(-ret, "sd_bus_message_read");

  std::string concatenatedString = str;
  Dout(dc::notice, "Received signal with concatenated string " << concatenatedString);

  return 0;
}

void on_signal_concatenated(dbus::MessageRead const& message)
{
  DoutEntering(dc::notice, "on_signal_concatenated(message)");
  sd_bus_error error;
  on_signal_concatenated(message, nullptr, &error);
}

// Open the gate to terminate application.
aithreadsafe::Gate gate;

int on_reply_concatenate(sd_bus_message* m, void* userdata, sd_bus_error* UNUSED_ARG(empty_error))
{
  DoutEntering(dc::notice, "on_reply_concatenate(" << m << ", " << userdata << ", emtpy_error)");
  dbus::MessageRead message(m);
  bool is_error = message.is_method_error();
  if (is_error)
  {
    dbus::Error dbus_error = message.get_error();
    Dout(dc::notice, "Message is an error: " << dbus_error);

    std::error_code error_code = dbus_error;
    Dout(dc::notice, "error_code = " << error_code << " [" << error_code.message() << "]");

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

void on_reply_concatenate(dbus::MessageRead const& message)
{
  DoutEntering(dc::notice, "on_reply_concatenate(message)");
  sd_bus_error error;
  on_reply_concatenate(message, nullptr, &error);
}

// Specialize boost::intrusive_ptr<task::DBusConnection> to track its instances.
DECLARE_TRACKED_BOOST_INTRUSIVE_PTR(task::DBusConnection)

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
  Debug(thread_pool.set_color_functions([](int color){ std::string code{"\e[30m"}; code[3] = '1' + color; return code; }));
  // And the thread pool queues.
  [[maybe_unused]] AIQueueHandle high_priority_queue   = thread_pool.new_queue(queue_capacity, reserved_threads);
  [[maybe_unused]] AIQueueHandle medium_priority_queue = thread_pool.new_queue(queue_capacity, reserved_threads);
                   AIQueueHandle low_priority_queue    = thread_pool.new_queue(queue_capacity);

  // Main application begin.
  try
  {
    // Set up the I/O event loop.
    evio::EventLoop event_loop(low_priority_queue, "\e[36m", "\e[0m");
    resolver::Scope resolver_scope(low_priority_queue, false);

    using statefultask::create;

    // Brokers are not singletons, but this object could be shared between multiple threads.
    auto broker = create<task::Broker<task::DBusConnection>>(CWDEBUG_ONLY(true));
    // The broker never finishes, unless abort() is called on it.
    broker->run(low_priority_queue);

    dbus::DBusConnectionBrokerKey broker_key;
    broker_key.request_service_name("com.alinoe.concatenator");

    // Set service name, object name, interface and method.
    dbus::Destination const destination("org.sdbuscpp.concatenator", "/org/sdbuscpp/concatenator", "org.sdbuscpp.Concatenator", "concatenate");
    dbus::Destination const signal_match("org.sdbuscpp.concatenator", "/org/sdbuscpp/concatenator", "org.sdbuscpp.Concatenator", "concatenated");

    // Let's subscribe for the 'concatenated' signals
    {
      auto dbus_match_signal = create<task::DBusMatchSignal>(broker, CWDEBUG_ONLY(true));
      dbus_match_signal->set_signal_match(&signal_match);
      dbus_match_signal->set_match_callback([&](dbus::MessageRead const& message) { on_signal_concatenated(message); });
      dbus_match_signal->run([](bool success){ Dout(dc::notice, "task::DBusMatchSignal " << (success ? "successful!" : "failed!")); });
    }

    std::vector<int32_t> const numbers = {1, 2, 3};
    std::string const separator = ":";

    // Invoke concatenate on given interface of the object.
    {
      auto dbus_method_call = create<task::DBusMethodCall>(CWDEBUG_ONLY(true));
      // It's ok to pass broker and destination as a pointers here, because their life time is longer than the life time of dbus_method_call.
      dbus_method_call->set_destination(broker, &broker_key, &destination);
      // Pass numbers and separator by reference, because their life time is longer than the life time of dbus_method_call.
      dbus_method_call->set_params_callback([&](dbus::Message& message) { message.append(numbers.begin(), numbers.end()).append(separator); });
      dbus_method_call->set_reply_callback([](dbus::MessageRead const& message) { on_reply_concatenate(message); });
      dbus_method_call->run([](bool success){ Dout(dc::notice, "task::DBusMethodCall " << (success ? "successful!" : "failed!")); });
    }

    // Invoke concatenate again, this time with no numbers and we shall get an error
    {
      auto dbus_method_call = create<task::DBusMethodCall>(CWDEBUG_ONLY(true));
      // It's ok to pass broker and destination as a pointers here, because their life time is longer than the life time of dbus_method_call.
      dbus_method_call->set_destination(broker, &broker_key, &destination);
      // Pass no numbers.
      dbus_method_call->set_params_callback([&](dbus::Message& message) { message.append(numbers.begin(), numbers.begin()).append(separator); });
      dbus_method_call->set_reply_callback([](dbus::MessageRead const& message) { on_reply_concatenate(message); });
      dbus_method_call->run([](bool success){ Dout(dc::notice, "task::DBusMethodCall " << (success ? "successful!" : "failed!")); });
    }

    // Wait until we received the error for the last method call.
    gate.wait();

    // Stop the broker task.
    broker->abort();

    // Print stuff...
    tracked::intrusive_ptr<task::DBusConnection>::for_each([](tracked::intrusive_ptr<task::DBusConnection> const* p){ Dout(dc::notice, p); });

    // Application terminated cleanly.
    event_loop.join();
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error << " [caught in client_side_concatenate_test.cxx].");
  }

  Dout(dc::notice, "Leaving main()");
}
