cmake_minimum_required(VERSION 3.12...3.16)

project(dbus-task
  DESCRIPTION "AIStatefulTask submodule - D-Bus support"
  LANGUAGES CXX
)

#==============================================================================
# DEPENDENCIES
#

find_package(sdbus-c++ REQUIRED)

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
    "DBusClient.cxx"
    "DBusClient.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(dbus-task_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
)

# Require support for C++17.
target_compile_features(dbus-task_ObjLib
  PUBLIC cxx_std_17
)

# Set link dependencies.
target_link_libraries(dbus-task_ObjLib
  PUBLIC
    AICxx::socket-task
  PRIVATE
    farmhash::farmhash
)

# Create an ALIAS target.
add_library(AICxx::dbus-task ALIAS dbus-task_ObjLib)

add_executable(server_side_concatenate_test server_side_concatenate_test.cxx)
target_compile_features(server_side_concatenate_test PUBLIC cxx_std_17)
target_link_libraries(server_side_concatenate_test PRIVATE SDBusCpp::sdbus-c++)

add_executable(client_side_concatenate_test client_side_concatenate_test.cxx)
target_compile_features(client_side_concatenate_test PUBLIC cxx_std_17)
target_link_libraries(client_side_concatenate_test PRIVATE SDBusCpp::sdbus-c++)
