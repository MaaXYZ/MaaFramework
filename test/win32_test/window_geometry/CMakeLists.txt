cmake_minimum_required(VERSION 3.28)
project(WindowGeometryTest LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Window geometry tests require Windows")
endif()

get_filename_component(MAA_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(MAA_UTILS_ROOT "${MAA_SOURCE_ROOT}/source/MaaUtils" CACHE PATH "MaaUtils source directory")
if(NOT EXISTS "${MAA_UTILS_ROOT}/include/MaaUtils/SafeWindows.hpp")
    message(FATAL_ERROR "Initialize source/MaaUtils or set MAA_UTILS_ROOT explicitly")
endif()
find_package(OpenCV REQUIRED COMPONENTS core imgproc)

add_executable(window_geometry_test window_geometry_test.cpp)
target_compile_features(window_geometry_test PRIVATE cxx_std_20)
target_compile_definitions(window_geometry_test PRIVATE UNICODE _UNICODE NOMINMAX)
target_include_directories(window_geometry_test PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/logging"
    "${MAA_SOURCE_ROOT}/include"
    "${MAA_SOURCE_ROOT}/source/include"
    "${MAA_SOURCE_ROOT}/source/MaaWin32ControlUnit"
    "${MAA_UTILS_ROOT}/include"
    ${OpenCV_INCLUDE_DIRS})
target_link_libraries(window_geometry_test PRIVATE ${OpenCV_LIBS} User32 Gdi32 Dwmapi)
if(MSVC)
    target_compile_options(window_geometry_test PRIVATE /W4 /WX /utf-8)
endif()

enable_testing()
add_test(NAME window_geometry COMMAND window_geometry_test)
set_tests_properties(window_geometry PROPERTIES TIMEOUT 30)
option(MAA_WINDOW_GEOMETRY_DESKTOP_TESTS "Run desktop geometry and visible-screen pixel checks" OFF)
if(MAA_WINDOW_GEOMETRY_DESKTOP_TESTS)
    add_test(NAME window_geometry_desktop COMMAND window_geometry_test --desktop)
    set_tests_properties(window_geometry_desktop PROPERTIES LABELS desktop RUN_SERIAL TRUE TIMEOUT 30)
endif()
# Desktop checks are explicit: --desktop-geometry checks native rectangles;
# --desktop additionally requires the visible fixture's screen pixel to match.
