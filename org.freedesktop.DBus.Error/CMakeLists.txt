# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(DBus_OrgFreedesktopDBusError_ObjLib OBJECT)

# The list of source files.
target_sources(DBus_OrgFreedesktopDBusError_ObjLib
  PRIVATE
    "Errors.cxx"
    "Errors.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(DBus_OrgFreedesktopDBusError_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
)

# Require support for C++17.
target_compile_features(DBus_OrgFreedesktopDBusError_ObjLib
  PUBLIC cxx_std_17
)

target_link_libraries(DBus_OrgFreedesktopDBusError_ObjLib
  PUBLIC
    magic_enum::magic_enum
)

# Create an ALIAS target.
add_library(AICxx::dbus-task::OrgFreedesktopDBusError ALIAS DBus_OrgFreedesktopDBusError_ObjLib)
