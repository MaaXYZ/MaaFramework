file(GLOB_RECURSE maa_linux_control_unit_src *.h *.hpp *.cpp Wayland/Protocol/*.c)
file(GLOB_RECURSE maa_linux_control_unit_header ${MAA_PUBLIC_INC}/MaaControlUnit/LinuxControlUnitAPI.h ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)
file(GLOB_RECURSE maa_linux_control_unit_compat_src Compat/*.cpp)
list(REMOVE_ITEM maa_linux_control_unit_src ${maa_linux_control_unit_compat_src})
set(maa_linux_control_unit_src_c99
        ${CMAKE_CURRENT_SOURCE_DIR}/Manager/LinuxControlUnitMgr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Screencap/PipeWireScreencap.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Screencap/PipeWireScreencap.h
)

add_library(MaaLinuxControlUnit SHARED ${maa_linux_control_unit_src} ${maa_linux_control_unit_header})
add_library(MaaWlRootsControlUnit SHARED ${CMAKE_CURRENT_SOURCE_DIR}/Compat/WlRootsControlUnitCompat.cpp)
add_library(MaaKWinControlUnit SHARED ${CMAKE_CURRENT_SOURCE_DIR}/Compat/KWinControlUnitCompat.cpp)

target_include_directories(MaaLinuxControlUnit
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})
target_include_directories(MaaWlRootsControlUnit
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})
target_include_directories(MaaKWinControlUnit
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

# Wayland and FFI
find_library(Wayland_LIBS wayland-client)
find_library(FFI_LIBS ffi)
target_link_libraries(MaaLinuxControlUnit PRIVATE MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS} Boost::system ${Wayland_LIBS} ${FFI_LIBS})
target_link_libraries(MaaWlRootsControlUnit PRIVATE MaaUtils MaaLinuxControlUnit ${OpenCV_LIBS})
target_link_libraries(MaaKWinControlUnit PRIVATE MaaUtils MaaToolkit MaaLinuxControlUnit ${OpenCV_LIBS})

# PipeWire
set(PIPEWIRE_ABS_LIB)
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    pkg_check_modules(PIPEWIRE QUIET libpipewire-0.3)
endif ()
if (PIPEWIRE_FOUND)
    # Search inside the sysroot (default CMAKE_FIND_ROOT_PATH behavior). In CI,
    # the ARM64 .deb packages are extracted and injected directly into the x-tools
    # sysroot, so the libraries are found naturally under <sysroot>/usr/lib/.
    find_library(PIPEWIRE_ABS_LIB
            NAMES pipewire-0.3
            PATHS ${PIPEWIRE_LIBRARY_DIRS} /usr/lib)
endif ()
if (PIPEWIRE_ABS_LIB)
    message(STATUS "MaaLinuxControlUnit: PipeWire absolute path: ${PIPEWIRE_ABS_LIB}")
    target_include_directories(MaaLinuxControlUnit PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
    target_link_libraries(MaaLinuxControlUnit PRIVATE ${PIPEWIRE_ABS_LIB})
else ()
    message(FATAL_ERROR "MaaLinuxControlUnit: PipeWire not found on this platform")
endif ()

set_source_files_properties(${maa_linux_control_unit_src_c99}
        PROPERTIES COMPILE_OPTIONS "-Wno-c99-extensions;-Wno-gnu-statement-expression-from-macro-expansion;-Wno-gnu-zero-variadic-macro-arguments;-Wno-gnu-case-range;-Wno-sign-compare"
)

target_compile_definitions(MaaLinuxControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)
target_compile_definitions(MaaWlRootsControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)
target_compile_definitions(MaaKWinControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaLinuxControlUnit MaaUtils)
add_dependencies(MaaWlRootsControlUnit MaaLinuxControlUnit)
add_dependencies(MaaKWinControlUnit MaaToolkit MaaLinuxControlUnit)

install(
        TARGETS MaaLinuxControlUnit MaaWlRootsControlUnit MaaKWinControlUnit
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_linux_control_unit_src} ${maa_linux_control_unit_compat_src})
