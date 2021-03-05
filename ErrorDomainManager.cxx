#include "sys.h"
#include "ErrorDomainManager.h"

namespace {
[[maybe_unused]] SingletonInstance<dbus::ErrorDomainManager> dummy;
} // namespace
