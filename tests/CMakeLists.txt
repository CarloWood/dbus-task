add_executable(server_side_concatenate_test EXCLUDE_FROM_ALL server_side_concatenate_test.cxx)
target_link_libraries(server_side_concatenate_test PRIVATE PkgConfig::Libsystemd AICxx::utils AICxx::cwds)

add_executable(client_side_concatenate_test EXCLUDE_FROM_ALL client_side_concatenate_test.cxx org.sdbuscpp.Concatenator.Error/Errors.cxx)
target_link_libraries(client_side_concatenate_test PRIVATE AICxx::dbus-task::OrgFreedesktopDBusError AICxx::dbus-task::SystemErrors AICxx::resolver-task farmhash::farmhash dns::dns AICxx::dbus-task ${AICXX_OBJECTS_LIST})

add_executable(error_test error_test.cxx org.sdbuscpp.Concatenator.Error/Errors.cxx)
target_link_libraries(error_test PRIVATE AICxx::dbus-task AICxx::dbus-task::OrgFreedesktopDBusError AICxx::dbus-task::SystemErrors ${AICXX_OBJECTS_LIST})
