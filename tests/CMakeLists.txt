add_executable(server_side_concatenate_test EXCLUDE_FROM_ALL server_side_concatenate_test.cxx)
target_compile_features(server_side_concatenate_test PUBLIC cxx_std_17)
target_link_libraries(server_side_concatenate_test PRIVATE PkgConfig::Libsystemd AICxx::utils AICxx::cwds)

add_executable(client_side_concatenate_test EXCLUDE_FROM_ALL client_side_concatenate_test.cxx)
target_compile_features(client_side_concatenate_test PUBLIC cxx_std_17)
target_link_libraries(client_side_concatenate_test PRIVATE AICxx::dbus-task::OrgFreedesktopDBusError AICxx::dbus-task::SystemErrors AICxx::resolver-task farmhash::farmhash dns::dns AICxx::dbus-task ${AICXX_OBJECTS_LIST})
