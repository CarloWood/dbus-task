# Include binary directory in include path, because that's where the generated header file resides.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(DBus_SystemErrors_ObjLib OBJECT)

# The list of source files.
target_sources(DBus_SystemErrors_ObjLib
  PRIVATE
    "Errors.cxx"
    "Errors.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(DBus_SystemErrors_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
)

# Require support for C++17.
target_compile_features(DBus_SystemErrors_ObjLib
  PUBLIC cxx_std_17
)

target_link_libraries(DBus_SystemErrors_ObjLib
  PUBLIC
    magic_enum::magic_enum
)

# Create an ALIAS target.
add_library(AICxx::dbus-task::SystemErrors ALIAS DBus_SystemErrors_ObjLib)

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/SystemErrors.h
  COMMENT "Generating SystemErrors.h..."
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND ./generate_SystemErrors.sh
  DEPENDS generate_SystemErrors.sh
)

add_custom_target(GenerateSystemErrors
  DEPENDS SystemErrors.h
)

# Add dependency on generated SystemErrors.h.
add_dependencies(DBus_SystemErrors_ObjLib GenerateSystemErrors)
