#include "sys.h"
#include "dbus-task/Connection.h"
#include "dbus-task/Error.h"
#include "dbus-task/ErrorCategory.h"
#include "dbus-task/System.Error/Errors.h"
#include "dbus-task/org.freedesktop.DBus.Error/Errors.h"
#include "statefultask/AIStatefulTask.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "evio/EventLoop.h"
#include "resolver-task/DnsResolver.h"
#include "threadsafe/Gate.h"
#include "threadpool/AIThreadPool.h"
#include "utils/debug_ostream_operators.h"
#include "debug.h"

#include <systemd/sd-bus.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

//---------------------------------------------------------------------------
// org.sdbuscpp.Concatenator.Error/Errors.h

namespace dbus::errors {
namespace org::sdbuscpp::Concatenator::Error {

// Errors defined for org.sdbuscpp.Concatenator.Error.*

// There is only one error defined.
enum Errors
{
  NoNumbers = 1
};

struct ErrorDomain : public ErrorDomainBase
{
  std::error_code get_error_code(std::string const& member_name) const override;
};

extern ErrorCategory<ErrorDomain, Errors> theErrorCategory;

// Functions that will be found using Argument-dependent lookup.
std::string to_string(Errors error);
std::ostream& operator<<(std::ostream& os, Errors error);
inline char const* get_domain(Errors) { return "DBus:org.sdbuscpp.Concatenator.Error"; }
inline std::error_code make_error_code(dbus::errors::org::sdbuscpp::Concatenator::Error::Errors ec) { return {ec, dbus::errors::org::sdbuscpp::Concatenator::Error::theErrorCategory}; }

} // namespace org::sdbuscpp::Concatenator::Error
} // namespace dbus::errors

// Register Errors as valid error code.
namespace std {
template<> struct is_error_code_enum<dbus::errors::org::sdbuscpp::Concatenator::Error::Errors> : true_type { };
} // namespace std

//---------------------------------------------------------------------------
// org.sdbuscpp.Concatenator.Error/Errors.cxx

#include <magic_enum.hpp>

namespace dbus::errors {
namespace org::sdbuscpp::Concatenator::Error {

std::string to_string(Errors error)
{
  return std::string{magic_enum::enum_name(error)};
}

std::ostream& operator<<(std::ostream& os, Errors error)
{
  os << to_string(error);
  return os;
}

std::error_code ErrorDomain::get_error_code(std::string const& member_name) const
{
  return make_error_code(*magic_enum::enum_cast<Errors>(member_name));
}

// Instantiation of the error category object.
ErrorCategory<ErrorDomain, Errors> theErrorCategory;

} // namespace org::sdbuscpp::Concatenator::Error
} // namespace dbus::errors

//---------------------------------------------------------------------------

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

    dbus::Error dbus_error = error;
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
    std::error_code ec5 = dbus::errors::System::Error::SE_ENOTSUP;
    std::error_code ec6 = dbus::errors::org::sdbuscpp::Concatenator::Error::NoNumbers;
    Dout(dc::notice, "ec5 = " << ec5 << " [" << ec5.message() << "]; ec6 = " << ec6 << " [" << ec6.message() << "]");

    dbus::Error dbe1{std::string("org.sdbuscpp.Concatenator.Error.NoNumbers"), std::string("some message")};
    dbus::Error dbe2{std::string("org.freedesktop.DBus.Error.Disconnected")};
    dbus::Error dbe3{std::string("System.Error.EAGAIN")};
    dbus::Error dbe4{std::string("org.freedesktop.DBus.Error.Something"), std::string("Non existing error")};
    std::error_code ec1 = static_cast<std::error_code>(dbe1);
    std::error_code ec2 = static_cast<std::error_code>(dbe2);
    std::error_code ec3 = static_cast<std::error_code>(dbe3);
    std::error_code ec4 = static_cast<std::error_code>(dbe4);
    Dout(dc::notice, "dbe1 = " << dbe1 << " (error_code = " << ec1 << " [" << ec1.message() << "]); " <<
        "dbe2 = " << dbe2 << " (error_code = " << ec2 << " [" << ec2.message() << "]); " <<
        "dbe3 = " << dbe3 << " (error_code = " << ec3 << " [" << ec3.message() << "]); " <<
        "dbe4 = " << dbe4 << " (error_code = " << ec4 << " [" << ec4.message() << "])");

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
    char const* serviceName = "org.sdbuscpp.concatenator";
    char const* objectPath      = "/org/sdbuscpp/concatenator";
//    auto concatenatorProxy      = sdbus::createProxy(connection->get_connection(), serviceName, objectPath);

    // Let's subscribe for the 'concatenated' signals
    char const* interfaceName = "org.sdbuscpp.Concatenator";
//    concatenatorProxy->registerSignalHandler(interfaceName, "concatenated", &onConcatenated);
//    concatenatorProxy->finishRegistration();

    sd_bus_match_signal_async(connection->get_bus(), nullptr, serviceName, objectPath, interfaceName, "concatenated", onConcatenated, nullptr, nullptr);

    std::vector<int32_t> numbers = {1, 2, 3};
    std::string separator    = ":";

    // Invoke concatenate on given interface of the object
    {
      //auto method = concatenatorProxy->createMethodCall(interfaceName, "concatenate");
      sd_bus_message* message;
      int ret;
      ret = sd_bus_message_new_method_call(connection->get_bus(), &message, serviceName, objectPath, interfaceName, "concatenate");
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
      ret = sd_bus_message_new_method_call(connection->get_bus(), &message, serviceName, objectPath, interfaceName, "concatenate");
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
