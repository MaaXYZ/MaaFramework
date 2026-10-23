add_executable(portal_test portal_test.cpp)
add_executable(pw_screencap_test pw_screencap_test.cpp)

target_link_libraries(portal_test MaaFramework MaaToolkit)
target_link_libraries(pw_screencap_test MaaFramework MaaToolkit)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(PORTAL_TEST_DBUS REQUIRED IMPORTED_TARGET dbus-1)
add_executable(portal_fd_test portal_fd_test.cpp)
target_link_libraries(portal_fd_test MaaToolkit PkgConfig::PORTAL_TEST_DBUS Threads::Threads)
