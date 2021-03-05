#include "sys.h"
#include "ErrorDomainManager.h"

namespace {
[[maybe_unused]] SingletonInstance<dbus::ErrorDomainManager> dummy;
} // namespace

namespace dbus {
std::error_code ErrorDomainBase::s_unknown_error{EBADRQC, std::system_category()};
std::error_code ErrorDomainBase::s_unknown_domain{EBADR, std::system_category()};
} // namespace dbus
