cmake_minimum_required(VERSION 3.21)
project(MAA)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_MAP_IMPORTED_CONFIG_DebWithRelDeps "DebWithRelDeps;Release;")

set(Boost_NO_WARN_NEW_VERSIONS 1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_MAADEPS "use third-party libraries built by MaaDeps" ON)
option(BUILD_SAMPLE "build a demo" OFF)
option(WITH_THRIFT "build with thrift" ON)
option(WITH_GRPC "build with protobuf and grpc" ON)

if(USE_MAADEPS)
    include(${PROJECT_SOURCE_DIR}/MaaDeps/maadeps.cmake)
endif()

# Basic compile and link configuration
include(${PROJECT_SOURCE_DIR}/cmake/config.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/version.cmake)
# include(${PROJECT_SOURCE_DIR}/cmake/nuget.cmake)

if(WITH_THRIFT AND NOT MAA_CROSSCOMPILE)
    include(${PROJECT_SOURCE_DIR}/cmake/thrift-gen.cmake)
    add_compile_definitions(WITH_THRIFT)
endif()
if(WITH_GRPC)
    include(${PROJECT_SOURCE_DIR}/cmake/grpc-gen.cmake)
    add_compile_definitions(WITH_GRPC)
endif()

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
find_package(Boost REQUIRED COMPONENTS system)
find_package(ZLIB REQUIRED)
find_package(fastdeploy_ppocr REQUIRED)
find_package(ONNXRuntime)
if(WITH_THRIFT AND NOT MAA_CROSSCOMPILE)
    find_package(Thrift CONFIG REQUIRED)
endif()
if(WITH_GRPC)
    find_package(protobuf CONFIG REQUIRED)
    find_package(gRPC CONFIG REQUIRED)
endif()

add_subdirectory(assets)
add_subdirectory(3rdparty)
add_subdirectory(source)

if(BUILD_SAMPLE)
    add_subdirectory(sample/cpp)
endif()

if(USE_MAADEPS)
    maadeps_install(bin)
endif()
