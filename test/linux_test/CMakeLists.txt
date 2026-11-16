add_executable(portal_test portal_test.cpp)
add_executable(pw_screencap_test pw_screencap_test.cpp)

target_link_libraries(portal_test MaaFramework MaaToolkit)
target_link_libraries(pw_screencap_test MaaFramework MaaToolkit)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(PORTAL_TEST_DBUS REQUIRED IMPORTED_TARGET dbus-1)
pkg_check_modules(PORTAL_TEST_GIO REQUIRED IMPORTED_TARGET gio-2.0)
add_executable(portal_fd_test portal_fd_test.cpp)
target_link_libraries(portal_fd_test MaaToolkit PkgConfig::PORTAL_TEST_DBUS PkgConfig::PORTAL_TEST_GIO Threads::Threads)
add_test(NAME portal_cleanup COMMAND portal_fd_test)
set_tests_properties(portal_cleanup PROPERTIES TIMEOUT 180)

add_executable(manager_cleanup_test
    manager_cleanup_test.cpp
    "${CMAKE_SOURCE_DIR}/source/MaaLinuxControlUnit/Manager/LinuxControlUnitMgr.cpp"
    "${CMAKE_SOURCE_DIR}/source/ControlUnitCommon/ControlUnitAPI.cpp")
# Inject backend failures without a compositor, PipeWire server, or input device.
target_include_directories(manager_cleanup_test BEFORE PRIVATE "${CMAKE_CURRENT_LIST_DIR}/manager_cleanup/support")
target_include_directories(manager_cleanup_test PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/source/include"
    "${CMAKE_SOURCE_DIR}/source/MaaLinuxControlUnit")
target_link_libraries(manager_cleanup_test PRIVATE ${OpenCV_LIBS})
add_test(NAME manager_cleanup COMMAND manager_cleanup_test)
set_tests_properties(manager_cleanup PROPERTIES TIMEOUT 30)
