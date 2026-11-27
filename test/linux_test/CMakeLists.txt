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
    "${CMAKE_SOURCE_DIR}/source/MaaLinuxControlUnit/Manager/LinuxControlUnitMgr.cpp")
# Inject backend failures without a compositor, PipeWire server, or input device.
target_include_directories(manager_cleanup_test BEFORE PRIVATE "${CMAKE_CURRENT_LIST_DIR}/manager_cleanup/support")
target_include_directories(manager_cleanup_test PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/source/include"
    "${CMAKE_SOURCE_DIR}/source/MaaLinuxControlUnit")
target_link_libraries(manager_cleanup_test PRIVATE MaaControlUnit ${OpenCV_LIBS})
add_test(NAME manager_cleanup COMMAND manager_cleanup_test)
set_tests_properties(manager_cleanup PROPERTIES TIMEOUT 30)

# Manual benchmark: needs a live compositor with ext-image-copy-capture-v1
# (e.g. wlroots/sway >= 0.19), so it is not registered with CTest.
set(MAA_EXT_PROTOCOL_DIR "${CMAKE_SOURCE_DIR}/source/MaaLinuxControlUnit/Wayland/Protocol")
find_library(ExtBench_Wayland_LIBS wayland-client)
find_library(ExtBench_FFI_LIBS ffi)
add_executable(ext_image_screencap_bench
    ext_image_screencap_bench.cpp
    "${MAA_EXT_PROTOCOL_DIR}/ext-image-capture-source-priv.c"
    "${MAA_EXT_PROTOCOL_DIR}/ext-image-copy-capture-priv.c"
    "${MAA_EXT_PROTOCOL_DIR}/ext-foreign-toplevel-list-priv.c")
target_include_directories(ext_image_screencap_bench PRIVATE "${MAA_EXT_PROTOCOL_DIR}")
# wayland-client.h lives in the MaaDeps vcpkg install prefix, next to OpenCV.
target_include_directories(ext_image_screencap_bench SYSTEM PRIVATE
    "${MAADEPS_DIR}/vcpkg/installed/${MAADEPS_TRIPLET}/include")
target_link_libraries(ext_image_screencap_bench PRIVATE ${ExtBench_Wayland_LIBS} ${ExtBench_FFI_LIBS})
