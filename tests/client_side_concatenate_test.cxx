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
#include "utils/threading/Gate.h"
#include "threadpool/AIThreadPool.h"
#include "utils/debug_ostream_operators.h"
#include "debug.h"
#ifdef CWDEBUG
#include "cwds/tracked_intrusive_ptr.h"
#endif

#include <iostream>
#include <string>
#include <vector>

constexpr int loop_size = 3000;
std::atomic_int signal_counter(0);

namespace utils { using namespace threading; }

void on_signal_concatenated(dbus::MessageRead const& message)
{
  DoutEntering(dc::notice, "on_signal_concatenated(message)");

  std::string concatenatedString;
  message >> concatenatedString;
  Dout(dc::notice, "Received signal with concatenated string " << concatenatedString);
}

// Open the gate to terminate application.
utils::Gate gate;

void on_reply_concatenate(dbus::MessageRead const& message, std::string expected_result)
{
  DoutEntering(dc::notice, "on_reply_concatenate(message)");
  bool is_error = message.is_method_error();
  if (is_error)
  {
    dbus::Error dbus_error = message.get_error();
    Dout(dc::notice, "Message is an error: " << dbus_error);

    std::error_code error_code = dbus_error;
    Dout(dc::notice, "error_code = " << error_code << " [" << error_code.message() << "] (signal_counter = " << signal_counter << ")");

    if (signal_counter++ == loop_size - 1)
      gate.open();
  }
  else
  {
    std::string result;
    message >> result;
    Dout(dc::notice, "result = \"" << result << "\", expected_result = \"" << expected_result << "\".");
    assert(result == expected_result);
  }
}

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
                   AIQueueHandle low_priority_queue    = thread_pool.new_queue(100 * queue_capacity);

  // Main application begin.
  try
  {
    // Set up the I/O event loop.
    evio::EventLoop event_loop(low_priority_queue COMMA_CWDEBUG_ONLY("\e[36m", "\e[0m"));
    resolver::Scope resolver_scope(low_priority_queue, false);

    using statefultask::create;

    // Brokers are not singletons, but this object could be shared between multiple threads.
    auto broker = create<task::Broker<task::DBusConnection>>(CWDEBUG_ONLY(true));
    // The broker never finishes, unless abort() is called on it.
    broker->run(low_priority_queue);

    dbus::DBusConnectionBrokerKey broker_key;
//    broker_key.request_service_name("com.alinoe.concatenator");

    // Set service name, object name, interface and method.
    dbus::Destination const destination("org.sdbuscpp.concatenator", "/org/sdbuscpp/concatenator", "org.sdbuscpp.Concatenator", "concatenate");
    dbus::Destination const signal_match("org.sdbuscpp.concatenator", "/org/sdbuscpp/concatenator", "org.sdbuscpp.Concatenator", "concatenated");

    // Let's subscribe for the 'concatenated' signals
    {
      auto dbus_match_signal = create<task::DBusMatchSignal>(broker COMMA_CWDEBUG_ONLY(true));
      dbus_match_signal->set_signal_match(&signal_match);
      dbus_match_signal->set_match_callback([&](dbus::MessageRead const& message) { on_signal_concatenated(message); });
      dbus_match_signal->run([](bool success){ Dout(dc::notice, "task::DBusMatchSignal " << (success ? "successful!" : "failed!")); });
    }

    std::string const separator = ":";
    for (int k = 1; k < loop_size * 3; k += 3)
    {
      std::vector<int32_t> const numbers = {k, k + 1, k + 2};
      std::string const expected_result = std::to_string(k) + ":" + std::to_string(k + 1) + ":" + std::to_string(k + 2);

      // Invoke concatenate on given interface of the object.
      {
        auto dbus_method_call = create<task::DBusMethodCall>(CWDEBUG_ONLY(true));
        // It's ok to pass broker, broker_key and destination as a pointers here, because their life time is longer than the life time of dbus_method_call.
        dbus_method_call->set_destination(broker, &broker_key, &destination);
        // Pass separator by reference, because their life time is longer than the life time of dbus_method_call.
        dbus_method_call->set_params_callback([&separator, numbers](dbus::Message& message) { message.append(numbers.begin(), numbers.end()).append(separator); });
        dbus_method_call->set_reply_callback([expected_result](dbus::MessageRead const& message) { on_reply_concatenate(message, expected_result); });
        dbus_method_call->run(low_priority_queue, [](bool success){ Dout(dc::notice, "task::DBusMethodCall " << (success ? "successful!" : "failed!")); });
      }

      // Invoke concatenate again, this time with no numbers and we shall get an error
      {
        auto dbus_method_call = create<task::DBusMethodCall>(CWDEBUG_ONLY(true));
        // It's ok to pass broker and destination as a pointers here, because their life time is longer than the life time of dbus_method_call.
        dbus_method_call->set_destination(broker, &broker_key, &destination);
        // Pass no numbers.
        dbus_method_call->set_params_callback([&](dbus::Message& message) { message.append(numbers.begin(), numbers.begin()).append(separator); });
        dbus_method_call->set_reply_callback([](dbus::MessageRead const& message) { on_reply_concatenate(message, ""); });
        dbus_method_call->run(low_priority_queue, [](bool success){ Dout(dc::notice, "task::DBusMethodCall " << (success ? "successful!" : "failed!")); });
      }
    }

    // Wait until we received the error for the last method call.
    gate.wait();

    // Stop the broker task.
    broker->abort();
    broker.reset();

    // Print stuff...
    utils::InstanceCollections::dump();

    // Application terminated cleanly.
    event_loop.join();
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error << " [caught in client_side_concatenate_test.cxx].");
  }

  Dout(dc::notice, "Leaving main()");
}
