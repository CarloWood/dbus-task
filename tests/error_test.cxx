#include "sys.h"
#include "dbus-task/System.Error/Errors.h"
#include "dbus-task/org.freedesktop.DBus.Error/Errors.h"
#include "org.sdbuscpp.Concatenator.Error/Errors.h"
#include "dbus-task/Error.h"
#include "utils/AIAlert.h"
#include <sstream>
#include "debug.h"

// It is possible to create POD constants (this is merely a wrapper around a sd_bus_error).
static constexpr dbus::ErrorConst no_numbers_error = { SD_BUS_ERROR_MAKE_CONST("org.sdbuscpp.Concatenator.Error.NoNumbers", "some message") };
static constexpr dbus::ErrorConst EAGAIN_error = { SD_BUS_ERROR_MAKE_CONST("System.Error.EAGAIN", "Try again man") };

int main()
{
  Debug(debug::init());

  std::stringstream ss;

  // The error enums can be converted to a std::error_code.
  std::error_code ecA = dbus::errors::System::Error::SE_ENOTSUP;
  std::error_code ecB = dbus::errors::org::sdbuscpp::Concatenator::Error::NoNumbers;
  Dout(dc::notice, "ecA = " << ecA << " [" << ecA.message() << "]; ecB = " << ecB << " [" << ecB.message() << "]");

  ss << ecA << " [" << ecA.message() << "]";
  ASSERT(ss.str() == "DBus:System.Error:95 [ENOTSUP]");
  ss.str(std::string{});
  ss.clear();

  ss << ecB << " [" << ecB.message() << "]";
  ASSERT(ss.str() == "DBus:org.sdbuscpp.Concatenator.Error:1 [NoNumbers]");
  ss.str(std::string{});
  ss.clear();

  dbus::Error dbe1{no_numbers_error};
  // These constructors are explicit, so we must pass std::string. String literals are deliberately not supported.
  // Not using a second argument causes the error to not have a 'message' associated with it.
  dbus::Error dbe2{std::string("org.freedesktop.DBus.Error.Disconnected")};
  // Instead, one should define an ErrorConst first, using SD_BUS_ERROR_MAKE_CONST:
  // In this case it is not possible not to have a 'message'.
  dbus::Error dbe3{EAGAIN_error};
  // We can receive errors that we don't know.
  dbus::Error dbe4{std::string("org.freedesktop.DBus.Error.Something"), std::string("Non existing error")};
  // Even errors from unknown domains.
  dbus::Error dbe5{std::string("flup.wop.Haha")};

  // dbus::Error seamless converts into a std::error_code.
  std::error_code ec1 = dbe1;
  std::error_code ec2 = dbe2;
  std::error_code ec3 = dbe3;
  std::error_code ec4 = dbe4;
  std::error_code ec5 = dbe5;

  Dout(dc::notice,
      "dbe1 = " << dbe1 << " (error_code = " << ec1 << " [" << ec1.message() << "]); " <<
      "dbe2 = " << dbe2 << " (error_code = " << ec2 << " [" << ec2.message() << "]); " <<
      "dbe3 = " << dbe3 << " (error_code = " << ec3 << " [" << ec3.message() << "]); " <<
      "dbe4 = " << dbe4 << " (error_code = " << ec4 << " [" << ec4.message() << "]); " <<
      "dbe5 = " << dbe5 << " (error_code = " << ec5 << " [" << ec5.message() << "])");

  using namespace dbus::errors;

  ss << dbe1 << ' ' << ec1 << " [" << ec1.message() << "]";
  ASSERT(ss.str() == "{ \"org.sdbuscpp.Concatenator.Error.NoNumbers\" [some message] } DBus:org.sdbuscpp.Concatenator.Error:1 [NoNumbers]");
  //                      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^    ^^^^^^^^^^^^    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ ^__^^^^^^^^^___ value and name of the corresponding enum.
  //                             \__ name of the sd_bus_error            \__ message of the sd_bus_error     \__ domain name of the error category
  // The corresponding enum in this case is dbus::errors::org::sdbuscpp::Concatenator::Error::NoNumbers
  ASSERT(ec1.value() == org::sdbuscpp::Concatenator::Error::NoNumbers);
  ss.str(std::string{});
  ss.clear();

  ss << dbe2 << ' ' << ec2 << " [" << ec2.message() << "]";
  ASSERT(ss.str() == "{ \"org.freedesktop.DBus.Error.Disconnected\" } DBus:org.freedesktop.DBus.Error:17 [Disconnected]");
  ASSERT(ec2.value() == org::freedesktop::DBus::Error::Disconnected);
  ss.str(std::string{});
  ss.clear();

  ss << dbe3 << ' ' << ec3 << " [" << ec3.message() << "]";
  ASSERT(ss.str() == "{ \"System.Error.EAGAIN\" [Try again man] } DBus:System.Error:11 [EAGAIN]");
  // Because EAGAIN is a macro, the enum has a prefix:
  ASSERT(ec3.value() == System::Error::SE_EAGAIN);
  // But well...
  ASSERT(ec3.value() == EAGAIN);
  ss.str(std::string{});
  ss.clear();

  ss << dbe4 << ' ' << ec4 << " [" << ec4.message() << "]";
  ASSERT(ss.str() == "{ \"org.freedesktop.DBus.Error.Something\" [Non existing error] } system:56 [Invalid request code]");
  //                                                                                    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  //                                                                                       \__ 'Something' doesn't exist.
  ASSERT(ec4.value() == EBADRQC);
  ss.str(std::string{});
  ss.clear();

  ss << dbe5 << ' ' << ec5 << " [" << ec5.message() << "]";
  ASSERT(ss.str() == "{ \"flup.wop.Haha\" } system:53 [Invalid request descriptor]");
  ASSERT(ec5.value() == EBADR);
  ss.str(std::string{});
  ss.clear();

  Dout(dc::notice, "Success.");
}
