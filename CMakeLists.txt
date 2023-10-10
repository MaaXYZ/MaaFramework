cmake_minimum_required(VERSION 3.21)
project(MAA)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_MAP_IMPORTED_CONFIG_DebWithRelDeps "DebWithRelDeps;Release;")
set(Boost_NO_WARN_NEW_VERSIONS 1)

option(BUILD_SAMPLE "build a demo" OFF)
option(USE_MAADEPS "use third-party libraries built by MaaDeps" ON)
option(WITH_THRIFT "build with thrift" ON)
option(WITH_GRPC "build with protobuf and grpc" ON)
option(BUILD_JAVASCRIPT "build JavaScript binding extras" OFF)

include(${PROJECT_SOURCE_DIR}/cmake/config.cmake) # Basic compile & link configuration
include(${PROJECT_SOURCE_DIR}/cmake/assets.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/version.cmake)
# include(${PROJECT_SOURCE_DIR}/cmake/nuget.cmake)
if(WITH_THRIFT)
    include(${PROJECT_SOURCE_DIR}/cmake/thrift-gen.cmake)
endif(WITH_THRIFT)

if(USE_MAADEPS)
    include(${PROJECT_SOURCE_DIR}/MaaDeps/maadeps.cmake)
endif()
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
find_package(Boost REQUIRED COMPONENTS system)
find_package(ZLIB REQUIRED)
find_package(fastdeploy_ppocr REQUIRED)
if(WITH_THRIFT)
    find_package(Thrift CONFIG REQUIRED)
endif(WITH_THRIFT)
if(WITH_GRPC)
    find_package(protobuf CONFIG REQUIRED)
    find_package(gRPC CONFIG REQUIRED)
endif(WITH_GRPC)
find_package(ONNXRuntime)

add_subdirectory(3rdparty)
add_subdirectory(source)

if (BUILD_SAMPLE)
    add_subdirectory(sample/cpp)
endif (BUILD_SAMPLE)

if(USE_MAADEPS)
    maadeps_install(bin)
endif()
