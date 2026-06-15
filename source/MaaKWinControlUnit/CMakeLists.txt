# ---------------------------------------------------------------------------
# MaaKWinControlUnit — KWin/KDE PipeWire screencap + UInput control
#
# On Linux (non-Android) with PipeWire and D-Bus available, builds the full
# implementation. Otherwise builds a stub that returns errors at runtime.
# ---------------------------------------------------------------------------

# Source files that are always compiled (stub or full)
set(maa_kwin_control_unit_src
    ${CMAKE_CURRENT_SOURCE_DIR}/API/KWinControlUnitAPI.cpp
)

# Determine whether full implementation is possible
# Libraries are found inside the sysroot via default CMAKE_FIND_ROOT_PATH behavior.
# In CI, for aarch64 cross-compilation, the ARM64 .deb packages are downloaded and
# extracted directly into the x-tools sysroot under <sysroot>/usr/lib/, so CMake
# finds them naturally. For x86_64 native builds, pkg-config finds the system libs.
set(PIPEWIRE_ABS_LIB)
set(DBUS_ABS_LIB)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT ANDROID)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PIPEWIRE QUIET libpipewire-0.3)
        pkg_check_modules(DBUS QUIET dbus-1)
    endif()
    if(PIPEWIRE_FOUND AND DBUS_FOUND)
        # Search inside the sysroot (default CMAKE_FIND_ROOT_PATH behavior). In CI,
        # the ARM64 .deb packages are extracted and injected directly into the x-tools
        # sysroot, so the libraries are found naturally under <sysroot>/usr/lib/.
        find_library(PIPEWIRE_ABS_LIB
            NAMES pipewire-0.3
            PATHS ${PIPEWIRE_LIBRARY_DIRS} /usr/lib)
        find_library(DBUS_ABS_LIB
            NAMES dbus-1
            PATHS ${DBUS_LIBRARY_DIRS} /usr/lib)
    endif()
endif()

if(PIPEWIRE_ABS_LIB AND DBUS_ABS_LIB)
    message(STATUS "MaaKWinControlUnit: full implementation (absolute paths: ${PIPEWIRE_ABS_LIB}, ${DBUS_ABS_LIB})")

    list(APPEND maa_kwin_control_unit_src
        ${CMAKE_CURRENT_SOURCE_DIR}/Input/UInputController.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Manager/KWinControlUnitMgr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Screencap/PipeWireScreencap.cpp
    )

    file(GLOB_RECURSE maa_kwin_control_unit_header
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
    )

    add_library(MaaKWinControlUnit SHARED ${maa_kwin_control_unit_src} ${maa_kwin_control_unit_header})

    target_include_directories(MaaKWinControlUnit
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC}
        ${PIPEWIRE_INCLUDE_DIRS} ${DBUS_INCLUDE_DIRS})

    # Link with absolute paths — CMake will embed the full path in the linker command
    # instead of generating -l flags, so the linker bypasses the sysroot entirely.
    target_link_libraries(MaaKWinControlUnit PRIVATE MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS} Boost::system
        ${PIPEWIRE_ABS_LIB} ${DBUS_ABS_LIB})

    target_compile_definitions(MaaKWinControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

    # PipeWire/SPA headers use C99 compound literals, GNU case ranges, and other GNU extensions;
    # disable the relevant warnings for all files that include SPA headers.
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/Screencap/PipeWireScreencap.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Manager/KWinControlUnitMgr.cpp
        PROPERTIES COMPILE_OPTIONS "-Wno-c99-extensions;-Wno-gnu-statement-expression-from-macro-expansion;-Wno-gnu-zero-variadic-macro-arguments;-Wno-gnu-case-range;-Wno-sign-compare"
    )

    add_dependencies(MaaKWinControlUnit MaaUtils)

    install(
        TARGETS MaaKWinControlUnit
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
    )

    # -----------------------------------------------------------------------
    # Integration test (controller + screencap)
    # -----------------------------------------------------------------------
    add_executable(test_kwin_screencap ${CMAKE_CURRENT_SOURCE_DIR}/test_kwin_screencap.cpp)
    target_link_libraries(test_kwin_screencap PRIVATE MaaKWinControlUnit MaaUtils ${OpenCV_LIBS})
    target_include_directories(test_kwin_screencap PRIVATE ${MAA_PUBLIC_INC}
        ${OpenCV_INCLUDE_DIRS})
    target_compile_options(test_kwin_screencap PRIVATE -Wno-c11-extensions)
    target_link_options(test_kwin_screencap PRIVATE -Wl,--allow-shlib-undefined)
    add_dependencies(test_kwin_screencap MaaKWinControlUnit)

    install(TARGETS test_kwin_screencap RUNTIME DESTINATION bin)

    # -----------------------------------------------------------------------
    # Keyboard input test (key_down / key_up via uinput)
    # -----------------------------------------------------------------------
    add_executable(test_kwin_keyboard ${CMAKE_CURRENT_SOURCE_DIR}/test_kwin_keyboard.cpp)
    target_link_libraries(test_kwin_keyboard PRIVATE MaaKWinControlUnit MaaUtils ${OpenCV_LIBS})
    target_include_directories(test_kwin_keyboard PRIVATE ${MAA_PUBLIC_INC}
        ${OpenCV_INCLUDE_DIRS})
    target_compile_options(test_kwin_keyboard PRIVATE -Wno-c11-extensions)
    target_link_options(test_kwin_keyboard PRIVATE -Wl,--allow-shlib-undefined)
    add_dependencies(test_kwin_keyboard MaaKWinControlUnit)

    install(TARGETS test_kwin_keyboard RUNTIME DESTINATION bin)

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_kwin_control_unit_src} ${maa_kwin_control_unit_header})
else()
    message(STATUS "MaaKWinControlUnit: stub build (absolute PipeWire/D-Bus libraries not found on this platform)")

    add_library(MaaKWinControlUnit SHARED ${maa_kwin_control_unit_src})

    target_include_directories(MaaKWinControlUnit
        PRIVATE ${MAA_PUBLIC_INC} ${OpenCV_INCLUDE_DIRS})

    target_link_libraries(MaaKWinControlUnit PRIVATE MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS})

    target_compile_definitions(MaaKWinControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

    install(
        TARGETS MaaKWinControlUnit
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
    )
endif()
