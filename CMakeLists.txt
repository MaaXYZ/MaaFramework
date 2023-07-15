cmake_minimum_required(VERSION 3.21)
project(MAA)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

option(BUILD_SAMPLE "build a demo" ON)
option(BUILD_BUSYBOX "build the busybox" OFF)
# option(INSTALL_DEVEL "install development files" OFF)
# option(INSTALL_THIRD_LIBS "install third party libraries" ON)
option(USE_MAADEPS "use third-party libraries built by MaaDeps" ON)
option(BUILD_EXTENSION "build a extension fo Visual Studio Code" OFF)

if (BUILD_BUSYBOX)
    set(MAA_DEBUG_EXPORTS ON)
endif ()

include(${PROJECT_SOURCE_DIR}/cmake/config.cmake) # Basic compile & link configuration
include(${PROJECT_SOURCE_DIR}/cmake/assets.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/version.cmake)

if(USE_MAADEPS)
    include(${PROJECT_SOURCE_DIR}/MaaDeps/maadeps.cmake)
endif()

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio)
find_package(ZLIB REQUIRED)
find_package(MaaDerpLearning REQUIRED)
# find_package(asio REQUIRED)
find_package(ONNXRuntime)
# find_package(cpr CONFIG REQUIRED)

add_subdirectory(3rdparty)
add_subdirectory(source)

if (BUILD_EXTENSION)
    add_subdirectory(tools/extension/screencap)
endif (BUILD_EXTENSION)

if (BUILD_SAMPLE)
    add_subdirectory(sample/cpp)
endif (BUILD_SAMPLE)

if (BUILD_BUSYBOX)
    add_subdirectory(test/busybox)
endif (BUILD_BUSYBOX)

if( USE_MAADEPS)
    maadeps_install()
endif()

# 好像还没适配，先注释了
# if (APPLE)
#     include(${PROJECT_SOURCE_DIR}/cmake/macos.cmake)
# endif (APPLE)
