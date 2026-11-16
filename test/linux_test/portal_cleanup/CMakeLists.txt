cmake_minimum_required(VERSION 3.28)
project(PortalCleanupTest LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Portal cleanup tests require Linux")
endif()

get_filename_component(MAA_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)

add_executable(portal_fd_test
    ../portal_fd_test.cpp
    "${MAA_SOURCE_ROOT}/source/MaaToolkit/Portal/PortalHelper.cpp"
    "${MAA_SOURCE_ROOT}/source/MaaToolkit/API/MaaToolkitPortal.cpp")
target_compile_features(portal_fd_test PRIVATE cxx_std_20)
target_include_directories(portal_fd_test PRIVATE
    logging
    "${MAA_SOURCE_ROOT}/include"
    "${MAA_SOURCE_ROOT}/source/MaaToolkit")
target_link_libraries(portal_fd_test PRIVATE PkgConfig::DBUS PkgConfig::GIO Threads::Threads)

enable_testing()
add_test(NAME portal_cleanup COMMAND portal_fd_test)
set_tests_properties(portal_cleanup PROPERTIES TIMEOUT 180)
