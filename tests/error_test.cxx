#include "sys.h"
#include "dbus-task/System.Error/Errors.h"
#include "dbus-task/org.freedesktop.DBus.Error/Errors.h"
#include "org.sdbuscpp.Concatenator.Error/Errors.h"
#include "dbus-task/Error.h"
#include "utils/AIAlert.h"
#include <sstream>
#include "debug.h"

int main()
{
  Debug(debug::init());

  std::stringstream ss;

  std::error_code ec5 = dbus::errors::System::Error::SE_ENOTSUP;
  std::error_code ec6 = dbus::errors::org::sdbuscpp::Concatenator::Error::NoNumbers;
  Dout(dc::notice, "ec5 = " << ec5 << " [" << ec5.message() << "]; ec6 = " << ec6 << " [" << ec6.message() << "]");

  ss << ec5 << " [" << ec5.message() << "]";
  ASSERT(ss.str() == "DBus:System.Error:95 [ENOTSUP]");
  ss.str(std::string{});
  ss.clear();

  ss << ec6 << " [" << ec6.message() << "]";
  ASSERT(ss.str() == "DBus:org.sdbuscpp.Concatenator.Error:1 [NoNumbers]");
  ss.str(std::string{});
  ss.clear();

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

  ss << dbe1 << ' ' << ec1 << " [" << ec1.message() << "]";
  ASSERT(ss.str() == "{ \"org.sdbuscpp.Concatenator.Error.NoNumbers\" [some message] } DBus:org.sdbuscpp.Concatenator.Error:1 [NoNumbers]");
  ss.str(std::string{});
  ss.clear();

  ss << dbe2 << ' ' << ec2 << " [" << ec2.message() << "]";
  ASSERT(ss.str() == "{ \"org.freedesktop.DBus.Error.Disconnected\" } DBus:org.freedesktop.DBus.Error:17 [Disconnected]");
  ss.str(std::string{});
  ss.clear();

  ss << dbe3 << ' ' << ec3 << " [" << ec3.message() << "]";
  ASSERT(ss.str() == "{ \"System.Error.EAGAIN\" } DBus:System.Error:11 [EAGAIN]");
  ss.str(std::string{});
  ss.clear();

  ss << dbe4 << ' ' << ec4 << " [" << ec4.message() << "]";
  ASSERT(ss.str() == "{ \"org.freedesktop.DBus.Error.Something\" [Non existing error] } system:56 [Invalid request code]");
  ss.str(std::string{});
  ss.clear();

  Dout(dc::notice, "Success.");
}
