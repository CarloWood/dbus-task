cmake_minimum_required(VERSION 3.12...3.16)

project(dbus-task
  DESCRIPTION "AIStatefulTask submodule - D-Bus support"
  LANGUAGES CXX
)

#==============================================================================
# DEPENDENCIES
#

find_package(PkgConfig REQUIRED)
pkg_check_modules(Libsystemd libsystemd REQUIRED IMPORTED_TARGET)
find_package(magic_enum REQUIRED)

include(AICxxProject)

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
    "Client.cxx"
    "Client.h"
    "Error.cxx"
    "Error.h"
    "ErrorDomainManager.cxx"
    "ErrorDomainManager.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(dbus-task_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
    "${Libsystemd_INCLUDE_DIRS}"
)

# Require support for C++17.
target_compile_features(dbus-task_ObjLib
  PUBLIC cxx_std_17
)

# Set link dependencies.
target_link_libraries(dbus-task_ObjLib
  PUBLIC
    AICxx::statefultask
    magic_enum::magic_enum
    PkgConfig::Libsystemd
)

# Create an ALIAS target.
add_library(AICxx::dbus-task ALIAS dbus-task_ObjLib)

# Error categories.
add_subdirectory(org.freedesktop.DBus.Error)
add_subdirectory(System.Error)

add_executable(server_side_concatenate_test server_side_concatenate_test.cxx)
target_compile_features(server_side_concatenate_test PUBLIC cxx_std_17)
target_link_libraries(server_side_concatenate_test PRIVATE PkgConfig::Libsystemd AICxx::utils AICxx::cwds)

add_executable(client_side_concatenate_test client_side_concatenate_test.cxx)
target_compile_features(client_side_concatenate_test PUBLIC cxx_std_17)
target_link_libraries(client_side_concatenate_test PRIVATE AICxx::dbus-task::OrgFreedesktopDBusError AICxx::dbus-task::SystemErrors AICxx::resolver-task farmhash::farmhash dns::dns AICxx::dbus-task ${AICXX_OBJECTS_LIST})
