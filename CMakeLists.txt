cmake_minimum_required(VERSION 3.24)
project(MaaFw)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_MAP_IMPORTED_CONFIG_DebWithRelDeps "DebWithRelDeps;Release;")

set(Boost_NO_WARN_NEW_VERSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(USE_MAADEPS "use third-party libraries built by MaaDeps" ON)

option(WITH_ADB_CONTROLLER "build with adb controller" ON)
option(WITH_WIN32_CONTROLLER "build with win32 controller" ON)
option(WITH_DBG_CONTROLLER "build with debugging controller" ON)
option(WITH_THRIFT_CONTROLLER "build with thrift controller" OFF)

option(WITH_GRPC "build with protobuf and grpc" OFF)

option(BUILD_GRPC_CLI "build grpc CLI exec" OFF)

option(BUILD_HTTP_CLI "build http CLI exec" ON)

option(BUILD_SAMPLE "build a demo" OFF)

option(BUILD_PIPELINE_TESTING "build pipeline testing" OFF)
option(BUILD_GRPC_TESTING "build grpc testing" OFF)
option(BUILD_DLOPEN_TESTING "build dlopen testing" OFF)

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
if(NOT WITH_GRPC)
    message(STATUS "Grpc is disabled, disable BUILD_GRPC_CLI and BUILD_GRPC_TESTING")
    set(BUILD_GRPC_CLI OFF)
    set(BUILD_GRPC_TESTING OFF)
endif()

if(NOT WITH_DBG_CONTROLLER)
    message(STATUS "Dbg controller is disabled, disable BUILD_PIPELINE_TESTING")
    set(BUILD_PIPELINE_TESTING OFF)
endif()

if(WITH_WIN32_CONTROLLER AND NOT WIN32)
    message(STATUS "Not on Windows, disable WITH_WIN32_CONTROLLER")
    set(WITH_WIN32_CONTROLLER OFF)
endif()

if(WITH_THRIFT_CONTROLLER)
    include(${PROJECT_SOURCE_DIR}/cmake/thrift-gen.cmake)
endif()

if(WITH_GRPC)
    include(${PROJECT_SOURCE_DIR}/cmake/grpc-gen.cmake)
endif()

if (BUILD_HTTP_CLI)
    find_program(NODE_EXE NAMES node nodejs)
    if(NOT NODE_EXE)
        message(STATUS "NodeJS not found, disable BUILD_HTTP_CLI")
        set(BUILD_HTTP_CLI OFF)
    endif()
endif()

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
find_package(Boost REQUIRED COMPONENTS system)
find_package(ZLIB REQUIRED)
find_package(fastdeploy_ppocr REQUIRED)
find_package(ONNXRuntime)

if(WITH_THRIFT_CONTROLLER)
    find_package(Thrift CONFIG REQUIRED)
endif()

if(WITH_GRPC)
    find_package(protobuf CONFIG REQUIRED)
    find_package(gRPC CONFIG REQUIRED)
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

if(BUILD_GRPC_TESTING)
    add_subdirectory(test/grpc)
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
