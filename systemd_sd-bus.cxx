#include "sys.h"
#define SB_BUS_NO_WRAP
#include "systemd_sd-bus.h"
#include "debug.h"
#include "OneThreadAtATime.h"
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <mutex>

OneThreadAtATime dbus_critical_area;

#define SD_BUS_DEFINE_VOID(R, N, P, ...) \
  R wrap_##N P \
  { \
    DoutEntering(dc::notice|continued_cf, BOOST_PP_STRINGIZE(BOOST_PP_CAT(sd_, N)) "... "); \
    std::lock_guard<OneThreadAtATime> lk(dbus_critical_area); \
    sd_##N (__VA_ARGS__); \
    Dout(dc::finish, "done"); \
  }

#define SD_BUS_DEFINE_NON_VOID(R, N, P, ...) \
  R wrap_##N P \
  { \
    DoutEntering(dc::notice|continued_cf, BOOST_PP_STRINGIZE(BOOST_PP_CAT(sd_, N)) " = "); \
    std::lock_guard<OneThreadAtATime> lk(dbus_critical_area); \
    R ret2 = sd_##N (__VA_ARGS__); \
    Dout(dc::finish, ret2); \
    return ret2; \
  }

#define SD_BUS_DEFINE_ELIPSIS(R, N, P, ...) \
  R wrap_##N P \
  { \
    va_list ap; \
    va_start(ap, types); \
    DoutEntering(dc::notice|continued_cf, BOOST_PP_STRINGIZE(BOOST_PP_CAT(sd_, N)) " = "); \
    std::lock_guard<OneThreadAtATime> lk(dbus_critical_area); \
    R ret2 = sd_##N##v (__VA_ARGS__, ap); \
    Dout(dc::finish, ret2); \
    va_end(ap); \
    return ret2; \
  }

SD_BUS_FOREACH_VOID_FUNCTION(SD_BUS_DEFINE_VOID)
SD_BUS_FOREACH_NON_VOID_FUNCTION(SD_BUS_DEFINE_NON_VOID)
SD_BUS_FOREACH_ELIPSIS_FUNCTION(SD_BUS_DEFINE_ELIPSIS)
