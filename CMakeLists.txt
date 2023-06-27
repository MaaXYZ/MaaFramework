cmake_minimum_required(VERSION 3.21)
project(MAA)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

option(BUILD_TEST "build a demo" ON)
option(BUILD_BUSYBOX "build test utility" ON)
option(BUILD_XCFRAMEWORK "build xcframework for macOS app" OFF)
option(BUILD_UNIVERSAL "build both arm64 and x86_64 on macOS" OFF)
option(INSTALL_PYTHON "install python ffi" OFF)
option(INSTALL_RESOURCE "install resource" OFF)
option(INSTALL_DEVEL "install development files" OFF)
option(INSTALL_THIRD_LIBS "install third party libraries" ON)
option(USE_MAADEPS "use third-party libraries built by MaaDeps" ON)

include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)

if(USE_MAADEPS)
    include(${PROJECT_SOURCE_DIR}/MaaDeps/maadeps.cmake)
endif()

if (MSVC)
    add_compile_options("/utf-8")
    add_compile_options("/MP")
    add_compile_options("/W4;/WX")
    add_compile_options("/wd4127")  # conditional expression is constant
    add_compile_options("/Wv:19.35.32217")  # disable warning introduced after this version 

    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # FIXME: remove -Wno-unused-parameter
    add_compile_options("-Wall;-Werror;-Wextra;-Wpedantic;-Wno-missing-field-initializers;-Wno-unused-parameter")
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
        add_compile_options("-Wno-restrict")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message("clang")
endif ()

add_library(header_only_libraries INTERFACE)
target_include_directories(header_only_libraries INTERFACE 3rdparty/include)

file(GLOB_RECURSE maa_src source/*.cpp source/*.h source/*.hpp)

add_library(MaaFramework SHARED ${maa_src})

if (WIN32)
    #注意：相比VS版本缺少了 -D_CONSOLE -D_WINDLL 两项
    target_compile_definitions(MaaFramework PRIVATE MAA_DLL_EXPORTS _UNICODE UNICODE)
endif ()
if (BUILD_BUSYBOX)
    target_compile_definitions(MaaFramework PRIVATE MAA_DEBUG_DLL_EXPORTS)
endif ()
target_include_directories(MaaFramework PUBLIC include PRIVATE source)
set(MaaFramework_PUBLIC_HEADERS include/MaaAPI.h)
set(MaaFramework_INCLUDE include)
target_sources(MaaFramework PUBLIC ${MaaFramework_PUBLIC_HEADERS})
set_target_properties(MaaFramework PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    PUBLIC_HEADER "${MaaFramework_PUBLIC_HEADERS}"
)

if(UNIX)
    set_target_properties(MaaFramework PROPERTIES INSTALL_RPATH "$ORIGIN/")
endif()

if (BUILD_TEST)
    add_executable(sample sample/cpp/main.cpp)
    set_target_properties(sample PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )
    target_link_libraries(sample MaaFramework)
    target_include_directories(sample PUBLIC ${MaaFramework_INCLUDE})
endif (BUILD_TEST)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio)
find_package(ZLIB REQUIRED)
find_package(MaaDerpLearning REQUIRED)
# find_package(asio REQUIRED)
# find_package(ONNXRuntime)
# find_package(cpr CONFIG REQUIRED)

target_link_libraries(MaaFramework ${OpenCV_LIBS} MaaDerpLearning ZLIB::ZLIB header_only_libraries) #asio::asio ONNXRuntime::ONNXRuntime cpr::cpr

if(WIN32)
    target_link_libraries(MaaFramework ws2_32)
endif()

### busybox

if (BUILD_BUSYBOX)
    file(GLOB_RECURSE maabusybox_src test/busybox/*.cpp test/busybox/*.hpp)
    add_executable(maabusybox ${maabusybox_src})
    set_target_properties(maabusybox PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )
    target_compile_definitions(maabusybox PRIVATE MAA_DEBUG)
    target_link_libraries(maabusybox MaaFramework header_only_libraries)
    target_include_directories(maabusybox PUBLIC ${MaaFramework_INCLUDE} "source")
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/config ${CMAKE_CURRENT_BINARY_DIR}/MaaAgentBinary
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/sample/cpp/config ${CMAKE_CURRENT_BINARY_DIR}/config
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/MaaAgentBinary ${CMAKE_CURRENT_BINARY_DIR}/MaaAgentBinary
    )
    add_custom_target(maabusybox_res ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config ${CMAKE_CURRENT_BINARY_DIR}/MaaAgentBinary)
    set_property(TARGET maabusybox_res APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/config ${CMAKE_CURRENT_BINARY_DIR}/MaaAgentBinary)
    add_dependencies(maabusybox maabusybox_res)
endif ()

###

# clang 15之后有ranges
# if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
#     find_package(range-v3 REQUIRED)
#     target_link_libraries(MaaFramework range-v3::range-v3)
# endif ()

if(INSTALL_DEVEL)
    set(MaaFramework_install_extra_args PUBLIC_HEADER DESTINATION devel/include ARCHIVE DESTINATION devel/lib)
endif()

install(TARGETS MaaFramework
    RUNTIME DESTINATION .
    LIBRARY DESTINATION .
    PUBLIC_HEADER DESTINATION .
    ${MaaFramework_install_extra_args}
)

if(INSTALL_THIRD_LIBS AND USE_MAADEPS)
    maadeps_install()
endif()

if (INSTALL_PYTHON)
    install(DIRECTORY sample/python DESTINATION .)
endif (INSTALL_PYTHON)
if (INSTALL_RESOURCE)
    install(DIRECTORY resource DESTINATION .)
endif (INSTALL_RESOURCE)


if (APPLE)
    include(${PROJECT_SOURCE_DIR}/cmake/macos.cmake)
endif (APPLE)

# define MAA_VERSION from git
if (NOT DEFINED MAA_VERSION)
    find_package(Git)
endif ()
if (NOT DEFINED MAA_VERSION AND GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE err
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (result EQUAL 0)
        set(MAA_VERSION "${output}")
    else ()
        message(WARNING "git rev-parse returning ${result}, output:\n${err}")
    endif ()
endif ()
if (NOT MAA_VERSION)
    set(MAA_VERSION "DEBUG_VERSION")
endif ()
message(STATUS "MAA_VERSION=${MAA_VERSION}")
add_compile_definitions(MAA_VERSION="${MAA_VERSION}")
