cmake_minimum_required(VERSION 3.28)
project(ManagerCleanupTest LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Manager cleanup tests require Linux")
endif()

get_filename_component(MAA_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(MAA_UTILS_ROOT "${MAA_SOURCE_ROOT}/source/MaaUtils" CACHE PATH "MaaUtils source directory")
if(TARGET opencv_core)
    set(OPENCV_CORE_INCLUDE_DIR ${OpenCV_INCLUDE_DIRS})
    set(OPENCV_CORE_LIBRARY opencv_core)
else()
    find_path(OPENCV_CORE_INCLUDE_DIR opencv2/core/mat.hpp PATH_SUFFIXES opencv4 REQUIRED)
    find_library(OPENCV_CORE_LIBRARY NAMES opencv_core REQUIRED)
endif()

add_executable(manager_cleanup_test
    ../manager_cleanup_test.cpp
    "${MAA_SOURCE_ROOT}/source/MaaLinuxControlUnit/Manager/LinuxControlUnitMgr.cpp"
    "${MAA_SOURCE_ROOT}/source/ControlUnitCommon/ControlUnitAPI.cpp")
target_compile_features(manager_cleanup_test PRIVATE cxx_std_20)
# Inject backend failures without a compositor, PipeWire server, or input device.
target_include_directories(manager_cleanup_test BEFORE PRIVATE "${CMAKE_CURRENT_LIST_DIR}/support")
target_include_directories(manager_cleanup_test PRIVATE
    "${MAA_SOURCE_ROOT}/include"
    "${MAA_SOURCE_ROOT}/source/include"
    "${MAA_SOURCE_ROOT}/source/MaaLinuxControlUnit"
    "${MAA_UTILS_ROOT}/include")
target_include_directories(manager_cleanup_test SYSTEM PRIVATE ${OPENCV_CORE_INCLUDE_DIR})
target_link_libraries(manager_cleanup_test PRIVATE "${OPENCV_CORE_LIBRARY}")

enable_testing()
add_test(NAME manager_cleanup COMMAND manager_cleanup_test)
set_tests_properties(manager_cleanup PROPERTIES TIMEOUT 30)
