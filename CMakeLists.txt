cmake_minimum_required(VERSION 3.26)
project(MaaFw)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(Boost_NO_WARN_NEW_VERSIONS ON)

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON) see below
option(USE_MAADEPS "use third-party libraries built by MaaDeps" ON)

option(WITH_ADB_CONTROLLER "build with adb controller" ON)
option(WITH_WIN32_CONTROLLER "build with win32 controller" ON)
option(WITH_DBG_CONTROLLER "build with debugging controller" ON)
option(WITH_NODEJS_BINDING "build with nodejs binding" OFF)

option(BUILD_PICLI "build maa picli" ON)
option(BUILD_SAMPLE "build a demo" OFF)
option(BUILD_PIPELINE_TESTING "build pipeline testing" OFF)
option(BUILD_DLOPEN_TESTING "build dlopen testing" OFF)

option(ENABLE_CCACHE "enable ccache if possible" ON)

set(MAA_PUBLIC_INC ${CMAKE_SOURCE_DIR}/include)
set(MAA_PRIVATE_INC ${CMAKE_SOURCE_DIR}/source/include)

if(USE_MAADEPS)
    set(MAADEPS_DIR ${PROJECT_SOURCE_DIR}/3rdparty/MaaDeps)
    include(${MAADEPS_DIR}/maadeps.cmake)
endif()

# Basic compile and link configuration
include(${PROJECT_SOURCE_DIR}/cmake/config.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/version.cmake)

# Disable unavailable options
if(NOT WITH_DBG_CONTROLLER)
    message(STATUS "Dbg controller is disabled, disable BUILD_PIPELINE_TESTING")
    set(BUILD_PIPELINE_TESTING OFF)
endif()

if(WITH_WIN32_CONTROLLER AND NOT WIN32)
    message(STATUS "Not on Windows, disable WITH_WIN32_CONTROLLER")
    set(WITH_WIN32_CONTROLLER OFF)
endif()

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
find_package(Boost REQUIRED COMPONENTS system)
find_package(ZLIB REQUIRED)
find_package(fastdeploy_ppocr REQUIRED)
find_package(ONNXRuntime REQUIRED)
find_package(cppzmq REQUIRED)

find_program(CCACHE_PROG ccache)

if(CCACHE_PROG)
    message("Find ccache at ${CCACHE_PROG}")

    if(ENABLE_CCACHE)
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROG})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROG})
    endif()
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

if(USE_MAADEPS)
    maadeps_install(bin)
endif()

install(
    EXPORT MaaFrameworkTargets
    FILE MaaFramework.cmake
    NAMESPACE MaaFramework::
    DESTINATION "share/cmake/MaaFramework")

include(CMakePackageConfigHelpers)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in "MaaFrameworkConfig.cmake"
                              INSTALL_DESTINATION share/cmake/MaaFramework)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/MaaFrameworkConfig.cmake"
        # "${CMAKE_CURRENT_BINARY_DIR}/MaaFrameworkConfigVersion.cmake"
        DESTINATION share/cmake/MaaFramework)

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

# using subdirectory at the end of file to make sure the script is called after everything is installed.
add_subdirectory(cmake/post_install)
