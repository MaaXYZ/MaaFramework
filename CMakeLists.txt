cmake_minimum_required(VERSION 3.21)
project(MAA)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO "RelWithDebInfo;Release;")
set(Boost_NO_WARN_NEW_VERSIONS 1)

option(BUILD_SAMPLE "build a demo" OFF)
option(USE_BOOST_IO "use boost as platform io" OFF)
# option(BUILD_BUSYBOX "build the busybox" OFF)
# option(INSTALL_DEVEL "install development files" OFF)
# option(INSTALL_THIRD_LIBS "install third party libraries" ON)
option(USE_MAADEPS "use third-party libraries built by MaaDeps" ON)
option(WITH_THRIFT "build with thrift" ON)

include(${PROJECT_SOURCE_DIR}/cmake/config.cmake) # Basic compile & link configuration
include(${PROJECT_SOURCE_DIR}/cmake/assets.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/version.cmake)
if(WITH_THRIFT)
    include(${PROJECT_SOURCE_DIR}/cmake/thrift-gen.cmake)
endif(WITH_THRIFT)

if(USE_MAADEPS)
    include(${PROJECT_SOURCE_DIR}/MaaDeps/maadeps.cmake)
endif()
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio)
find_package(Boost REQUIRED COMPONENTS system url)
find_package(ZLIB REQUIRED)
find_package(MaaDerpLearning REQUIRED)
if(WITH_THRIFT)
    find_package(Thrift CONFIG REQUIRED)
endif(WITH_THRIFT)
find_package(ONNXRuntime)

# set_target_properties(${OpenCV_LIBS} PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)

add_subdirectory(3rdparty)
add_subdirectory(source)

if (BUILD_SAMPLE)
    add_subdirectory(sample/cpp)
endif (BUILD_SAMPLE)

# if (BUILD_BUSYBOX)
#     add_subdirectory(test/busybox)
# endif (BUILD_BUSYBOX)

if(USE_MAADEPS)
    maadeps_install(bin)
endif()

# 好像还没适配，先注释了
# if (APPLE)
#     include(${PROJECT_SOURCE_DIR}/cmake/macos.cmake)
# endif (APPLE)
