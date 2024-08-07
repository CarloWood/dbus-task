cmake_minimum_required(VERSION 3.15...3.30)

project(dbus-task
  DESCRIPTION "AIStatefulTask submodule - D-Bus support"
  LANGUAGES CXX
)

#==============================================================================
# DEPENDENCIES
#

include(AICxxProject)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Libsystemd libsystemd REQUIRED IMPORTED_TARGET)
find_package(magic_enum REQUIRED)

#==============================================================================
# BUILD PROJECT
#

# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(dbus-task_ObjLib OBJECT)

# Require at least support for C++17.
target_compile_features(dbus-task_ObjLib PUBLIC cxx_std_17)

# The list of source files.
target_sources(dbus-task_ObjLib
  PRIVATE
    "Connection.cxx"
    "Connection.h"
    "DBusConnection.cxx"
    "DBusConnection.h"
    "DBusHandleIO.h"
    "DBusHandleIO.cxx"
    "DBusMatchSignal.h"
    "DBusMatchSignal.cxx"
    "DBusMethodCall.cxx"
    "DBusMethodCall.h"
    "DBusObject.cxx"
    "DBusObject.h"
    "Error.cxx"
    "Error.h"
    "ErrorDomainManager.cxx"
    "ErrorDomainManager.h"
    "ErrorException.h"
    "Message.cxx"
    "Message.h"

    "systemd_sd-bus.cxx"
    "systemd_sd-bus.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(dbus-task_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
    "${Libsystemd_INCLUDE_DIRS}"
)

# Set link dependencies.
target_link_libraries(dbus-task_ObjLib
  PUBLIC
    AICxx::statefultask
    farmhash::farmhash
    magic_enum::magic_enum
    PkgConfig::Libsystemd
)

# Create an ALIAS target.
add_library(AICxx::dbus-task ALIAS dbus-task_ObjLib)

# Error categories.
add_subdirectory(org.freedesktop.DBus.Error)
add_subdirectory(System.Error)

# Tests.
add_subdirectory(tests)
