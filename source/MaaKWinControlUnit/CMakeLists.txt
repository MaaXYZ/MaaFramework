# Find PipeWire and D-Bus packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)
pkg_check_modules(DBUS1 REQUIRED IMPORTED_TARGET dbus-1)

file(GLOB_RECURSE maa_kwin_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_kwin_control_unit_header ${MAA_PUBLIC_INC}/MaaControlUnit/KWinControlUnitAPI.h ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)

add_library(MaaKWinControlUnit SHARED ${maa_kwin_control_unit_src} ${maa_kwin_control_unit_header})

target_include_directories(MaaKWinControlUnit
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC}
        ${PIPEWIRE_INCLUDE_DIRS} ${DBUS1_INCLUDE_DIRS})

target_link_libraries(MaaKWinControlUnit PRIVATE MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS} Boost::system
        PkgConfig::PIPEWIRE PkgConfig::DBUS1)

target_compile_definitions(MaaKWinControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

# PipeWire/SPA headers use C99 compound literals, GNU case ranges, and other GNU extensions;
# disable the relevant warnings for PipeWireScreencap.cpp only.
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/Screencap/PipeWireScreencap.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-c99-extensions;-Wno-gnu-statement-expression-from-macro-expansion;-Wno-gnu-zero-variadic-macro-arguments;-Wno-gnu-case-range"
)

add_dependencies(MaaKWinControlUnit MaaUtils)

install(
        TARGETS MaaKWinControlUnit
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
)

# ---------------------------------------------------------------------------
# Integration test (controller + screencap)
# ---------------------------------------------------------------------------
add_executable(test_kwin_screencap test_kwin_screencap.cpp)
target_link_libraries(test_kwin_screencap PRIVATE MaaKWinControlUnit MaaUtils)
target_include_directories(test_kwin_screencap PRIVATE ${MAA_PUBLIC_INC}
    ${OpenCV_INCLUDE_DIRS})
target_compile_options(test_kwin_screencap PRIVATE -Wno-c11-extensions)
add_dependencies(test_kwin_screencap MaaKWinControlUnit)

# Install test binary alongside the library
install(TARGETS test_kwin_screencap RUNTIME DESTINATION bin)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_kwin_control_unit_src})
