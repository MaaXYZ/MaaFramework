cmake_minimum_required(VERSION 3.28)

project(MaaFw)

set(CMAKE_CXX_SCAN_FOR_MODULES  OFF)

include(source/MaaUtils/MaaUtils.cmake)

option(WITH_ADB_CONTROLLER "build with adb controller" ON)
option(WITH_WIN32_CONTROLLER "build with win32 controller" ON)
option(WITH_DBG_CONTROLLER "build with debugging controller" OFF)
option(WITH_CUSTOM_CONTROLLER "build with custom controller" ON)
option(WITH_PLAYCOVER_CONTROLLER "build with PlayCover controller for macOS" ON)
option(WITH_NODEJS_BINDING "build with nodejs binding" OFF)
option(WITH_QUICKJS_BINDING "build with quickjs binding" OFF)
option(WITH_MAA_AGENT "build with MaaAgent" ON)

option(BUILD_PICLI "build maa picli" ON)
option(BUILD_SAMPLE "build a demo" OFF)
option(BUILD_PIPELINE_TESTING "build pipeline testing" OFF)
option(BUILD_DLOPEN_TESTING "build dlopen testing" OFF)
option(BUILD_NODE_TEST "build node test" OFF)

option(ENABLE_CCACHE "enable ccache if possible" ON)
option(ENABLE_HASH_VERSION "enable hash version from git" OFF)

# option(ENABLE_CPP20_MODULES "enable C++20 modules" ON)
set(MAA_PUBLIC_INC ${CMAKE_SOURCE_DIR}/include)
set(MAA_PRIVATE_INC ${CMAKE_SOURCE_DIR}/source/include)

# Disable unavailable options
if(NOT WITH_DBG_CONTROLLER)
    message(STATUS "Dbg controller is disabled, disable BUILD_PIPELINE_TESTING")
    set(BUILD_PIPELINE_TESTING OFF)
endif()

if(WITH_WIN32_CONTROLLER AND NOT WIN32)
    message(STATUS "Not on Windows, disable WITH_WIN32_CONTROLLER")
    set(WITH_WIN32_CONTROLLER OFF)
endif()

if(WITH_PLAYCOVER_CONTROLLER AND NOT APPLE)
    message(STATUS "Not on macOS, disable WITH_PLAYCOVER_CONTROLLER")
    set(WITH_PLAYCOVER_CONTROLLER OFF)
endif()

if(WITH_MAA_AGENT)
    find_package(cppzmq REQUIRED)
endif()

add_subdirectory(3rdparty)
add_subdirectory(source)
add_subdirectory(tools)

if(BUILD_SAMPLE)
    add_subdirectory(sample/cpp)
endif()

if(BUILD_PIPELINE_TESTING)
    add_subdirectory(test/pipeline)
    add_subdirectory(test/TestingDataSet)
endif()

if(BUILD_DLOPEN_TESTING)
    add_subdirectory(test/dlopen)
endif()

if(BUILD_NODE_TEST)
    add_subdirectory(tools/NodeTest)
endif()

maadeps_install(bin)

install(
    TARGETS MaaUtils
    EXPORT MaaFrameworkTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin # ARCHIVE DESTINATION lib
)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaUtils> DESTINATION symbol OPTIONAL)
endif()

install(
    EXPORT MaaFrameworkTargets
    FILE MaaFramework.cmake
    NAMESPACE MaaFramework::
    DESTINATION "share/cmake/MaaFramework")

if(WITH_MAA_AGENT)
    install(
        EXPORT MaaAgentServerTargets
        FILE MaaAgentServer.cmake
        NAMESPACE MaaAgentServer::
        DESTINATION "share/cmake/MaaAgentServer")
endif()

include(CMakePackageConfigHelpers)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in "MaaFrameworkConfig.cmake"
    INSTALL_DESTINATION share/cmake/MaaFramework)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in "MaaAgentServerConfig.cmake"
    INSTALL_DESTINATION share/cmake/MaaAgentServer)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/MaaFrameworkConfig.cmake"
    DESTINATION share/cmake/MaaFramework)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/MaaAgentServerConfig.cmake"
    DESTINATION share/cmake/MaaAgentServer)

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/export_targets.cmake)
    message("Load export_targets.cmake")
    include(${CMAKE_CURRENT_LIST_DIR}/export_targets.cmake)

    # cmake-format: off
    #
    # specify export compile commands for target like this
    #
    # if(TARGET sample_maapp_coro)
    # set_target_properties(sample_maapp_coro PROPERTIES EXPORT_COMPILE_COMMANDS ON)
    # endif()
    #
    # cmake-format: on
endif()
