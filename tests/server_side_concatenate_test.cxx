#include "sys.h"
#include "org.sdbuscpp.Concatenator.Error/Errors.h"
#include "dbus-task/DBusConnection.h"
#include "dbus-task/DBusConnectionBrokerKey.h"
#include "dbus-task/DBusObject.h"
#include "dbus-task/Interface.h"
#include "statefultask/AIStatefulTask.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "statefultask/Broker.h"
#include "evio/EventLoop.h"
#include "threadpool/AIThreadPool.h"
#include "resolver-task/DnsResolver.h"
#include "utils/AIAlert.h"
#include "utils/debug_ostream_operators.h"
#include "debug.h"
#ifdef CWDEBUG
#include "cwds/tracked_intrusive_ptr.h"
#endif

#include <iostream>
#include <string>
#include <vector>

#include <systemd/sd-bus.h>

static int method_concatenate(sd_bus_message* m, void* userdata, sd_bus_error* ret_error)
{
  DoutEntering(dc::notice, "method_concatenate(" << m << ", " << userdata << ", ret_error)");

  // Deserialize the collection of numbers from the message.
  std::vector<int> numbers;

  int ret = sd_bus_message_enter_container(m, 'a', "i");
  if (ret < 0)
    THROW_ALERTC(-ret, "sd_bus_message_enter_container");

  for (;;)
  {
    int n;
    ret = sd_bus_message_read(m, "i", &n);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_message_read");
    if (ret == 0)
      break;
    numbers.push_back(n);
  }

  ret = sd_bus_message_exit_container(m);
  if (ret < 0)
    THROW_ALERTC(-ret, "sd_bus_message_exit_container");

  if (numbers.empty())
  {
    sd_bus_error_set(ret_error, "org.sdbuscpp.Concatenator.Error.NoNumbers", "No numbers provided");
    return 0;
  }

  char const* separator;
  ret = sd_bus_message_read(m, "s", &separator);
  if (ret < 0)
    THROW_ALERTC(-ret, "sd_bus_message_read");

  std::string result;
  for (auto number : numbers) { result += (result.empty() ? std::string() : separator) + std::to_string(number); }

  ret = sd_bus_reply_method_return(m, "s", result.c_str());
  if (ret < 0)
    THROW_ALERTC(-ret, "sd_bus_reply_method_return");

#if 0
  // Emit 'concatenated' signal
  char const* objectPath    = "/org/sdbuscpp/concatenator";
  char const* interfaceName = "org.sdbuscpp.Concatenator";

  boost::intrusive_ptr<task::Broker<task::DBusConnection>> broker{static_cast<task::Broker<task::DBusConnection>*>(userdata)};
  broker.run(key, [](bool success){
      });
  boost::intrusive_ptr<TaskType const> run(statefultask::BrokerKey const& key, std::function<void(bool)>&& callback);
  ret = sd_bus_emit_signal(bus, objectPath, interfaceName, "concatenated", "s", result.c_str());
  if (ret < 0)
    THROW_ALERTC(-ret, "sd_bus_emit_signal");
#endif

  return 0;
}

// The vtable of our little object, implements the org.sdbuscpp.Concatenator interface.
static sd_bus_vtable const concatenator_vtable[] = {
  SD_BUS_VTABLE_START(0),
  SD_BUS_METHOD("concatenate", "ais", "s", method_concatenate, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_VTABLE_END
};

int main()
{
  Debug(debug::init());
  Dout(dc::notice, "Entering main()");

  // Create a AIMemoryPagePool object (must be created before thread_pool).
  [[maybe_unused]] AIMemoryPagePool mpp;

  // Set up the thread pool for the application.
  int const number_of_threads = 8;                      // Use a thread pool of 8 threads.
  int const max_number_of_threads = 16;                 // This can later dynamically be increased to 16 if needed.
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
    broker_key.request_service_name("org.sdbuscpp.concatenator");

    // Set service name, object name, interface and method.
    dbus::Interface const interface("org.sdbuscpp.concatenator", "/org/sdbuscpp/concatenator", "org.sdbuscpp.Concatenator");

    // Install the object.
    {
      auto dbus_object = create<task::DBusObject>(CWDEBUG_ONLY(true));
      // It's ok to pass broker, broker_key and interface as a pointers here, because their life time is longer than the life time of dbus_object.
      dbus_object->set_interface(broker, &broker_key, &interface);
      // The task::DBusObject stores a boost::intrusive_ptr to the broker, so it is save to pass
      // the broker as a void* userdata.
      dbus_object->add_vtable(concatenator_vtable, broker.get());
      dbus_object->run([](bool success){ Dout(dc::notice, "task::DBusObject " << (success ? "successful!" : "failed!")); });
    }

    // Allow the broker to establish a connection.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop the broker task.
//    broker->abort();

    Dout(dc::warning, "Leaving main thread scope -- does this cause program termination?");

    // Print stuff...
    Debug(tracked::intrusive_ptr<task::DBusConnection>::for_each([](tracked::intrusive_ptr<task::DBusConnection> const* p){ Dout(dc::notice, p); }));

    // Application terminated cleanly.
    event_loop.join();
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error << " [caught in server_side_concatenate_test.cxx].");
  }

  Dout(dc::notice, "Leaving main()");
}
