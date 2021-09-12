cmake_minimum_required(VERSION 3.12...3.16)

project(dbus-task
  DESCRIPTION "AIStatefulTask submodule - D-Bus support"
  LANGUAGES CXX
)

# Require support for C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Set SOURCE_DIR to the current directory.
set_property(
  TARGET dbus-task_ObjLib
  PROPERTY SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

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
