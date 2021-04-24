#include "sys.h"
#include "org.sdbuscpp.Concatenator.Error/Errors.h"
#include "dbus-task/DBusConnection.h"
#include "dbus-task/DBusConnectionBrokerKey.h"
#include "dbus-task/DBusObject.h"
#include "dbus-task/Error.h"
#include "dbus-task/Interface.h"
#include "statefultask/AIStatefulTask.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "statefultask/Broker.h"
#include "statefultask/AIStatefulTaskMutex.h"
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

#include "dbus-task/systemd_sd-bus.h"

namespace dbus {
static constexpr ErrorConst no_numbers_error = { SD_BUS_ERROR_MAKE_CONST("org.sdbuscpp.Concatenator.Error.NoNumbers", "No numbers provided") };
} // namespace dbus

class MyDBusObject : public task::DBusObject
{
  using DBusObject::DBusObject;

  bool object_callback(dbus::Message message) override;

  std::string concatenate(sd_bus* bus, std::vector<int32_t> const& numbers, std::string const& separator)
  {
    DoutEntering(dc::notice, "MyDBusObject::concatenate(" << numbers << ", " << "\"" << separator << "\")");
    if (numbers.empty())
      throw dbus::Error(dbus::no_numbers_error);
    std::string result;
    for (auto number : numbers)
      result += (result.empty() ? std::string() : separator) + std::to_string(number);

    // Send a signal that we catenated something successfully.
    // It is safe to call sd_bus_emit_signal because the connection that this bus is associated with is already locked:
    // this is a callback initiated from the sdbus library.
#ifdef CWDEBUG
    // This should never fail because bus (which should be message.get_bus(), the received method call) is expected
    // to be the same as the bus that was used to register the service to receive that method call in the first place.
    ASSERT(is_same_bus(bus));
    // This should never fail because we are in a callback initiated from the sdbus library.
//FIXME    ASSERT(is_self_locked());
#endif
    Dout(dc::notice, "Calling sd_bus_emit_signal(" << get_interface() << ", \"" << result << "\")");
    int ret = sd_bus_emit_signal(bus, get_interface()->object_path(), get_interface()->interface_name(), "concatenated", "s", result.c_str());
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_emit_signal");

    return result;
  }
};

bool MyDBusObject::object_callback(dbus::Message message)
{
  DoutEntering(dc::notice, "MyDBusObject::object_callback()");
  if (message.is_method_call("org.sdbuscpp.Concatenator", "concatenate"))
  {
    std::vector<int32_t> numbers;
    message >> std::back_insert_iterator(numbers);
    std::string separator;
    message >> separator;
    std::string result = concatenate(message.get_bus(), numbers, separator);
    message.reply_method_return(result);
    return true;
  }
  return false;
}

int main()
{
  Debug(debug::init());
  Dout(dc::notice, "Entering main()");

  // Create a AIMemoryPagePool object (must be created before thread_pool).
  AIMemoryPagePool mpp;

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
      auto dbus_object = create<MyDBusObject>(CWDEBUG_ONLY(true));
      // It's ok to pass broker, broker_key and interface as a pointers here, because their life time is longer than the life time of dbus_object.
      dbus_object->set_interface(broker, &broker_key, &interface);
      // The task::DBusObject stores a boost::intrusive_ptr to the broker, so it is save to pass
      // the broker as a void* userdata.
      dbus_object->set_userdata(broker.get());
      dbus_object->run([&dbus_object](bool success){ Dout(dc::notice, "task::DBusObject " << (success ? "successful!" : "failed!")); if (success) dbus_object->run(); });
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
