file(GLOB_RECURSE maa_toolkit_src *.h *.hpp *.cpp)

if(APPLE)
    file(GLOB_RECURSE maa_toolkit_objc_src *.mm)
    list(APPEND maa_toolkit_src ${maa_toolkit_objc_src})
endif()

file(GLOB_RECURSE maa_toolkit_header ${MAA_PUBLIC_INC}/MaaToolkit/*)

add_library(MaaToolkit SHARED ${maa_toolkit_src} ${maa_toolkit_header})

target_include_directories(
    MaaToolkit
    PUBLIC $<BUILD_INTERFACE:${MAA_PUBLIC_INC}> $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_compile_definitions(MaaToolkit PRIVATE MAA_TOOLKIT_EXPORTS)

target_link_libraries(MaaToolkit PRIVATE MaaUtils MaaFramework LibraryHolder Boost::system ${OpenCV_LIBS})

if (LINUX)
    set(DBUS_ABS_LIB)
    find_package(PkgConfig QUIET)
    if (PkgConfig_FOUND)
        pkg_check_modules(DBUS QUIET dbus-1)
    endif ()
    if (DBUS_FOUND)
        # Search inside the sysroot (default CMAKE_FIND_ROOT_PATH behavior). In CI,
        # the ARM64 .deb packages are extracted and injected directly into the x-tools
        # sysroot, so the libraries are found naturally under <sysroot>/usr/lib/.
        find_library(DBUS_ABS_LIB
                NAMES dbus-1
                PATHS ${DBUS_LIBRARY_DIRS} /usr/lib)
    endif ()
    if (DBUS_ABS_LIB)
        message(STATUS "MaaToolkit: DBus absolute path: ${DBUS_ABS_LIB}")
        target_include_directories(MaaToolkit PRIVATE ${DBUS_INCLUDE_DIRS})
        target_link_libraries(MaaToolkit PRIVATE ${DBUS_ABS_LIB})
    else ()
        message(FATAL_ERROR "MaaToolkit: PipeWire not found on this platform")
    endif ()
    find_library(Wayland_LIBS wayland-client)
    find_library(FFI_LIBS ffi)
    target_link_libraries(MaaToolkit PRIVATE pthread dl ${Wayland_LIBS} ${FFI_LIBS})
endif ()

if(APPLE)
    # macOS frameworks for ScreenCaptureKit
    find_library(SCREEN_CAPTURE_KIT ScreenCaptureKit REQUIRED)
    find_library(COCOA Cocoa REQUIRED)
    target_link_libraries(MaaToolkit PRIVATE ${SCREEN_CAPTURE_KIT} ${COCOA})
endif()

add_dependencies(MaaToolkit MaaUtils MaaFramework LibraryHolder)

install(
    TARGETS MaaToolkit
    EXPORT MaaFrameworkTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)

if(WIN32)
    install(
        FILES $<TARGET_PDB_FILE:MaaToolkit>
        DESTINATION symbol
        OPTIONAL)
endif()

install(DIRECTORY "${MAA_PUBLIC_INC}/MaaToolkit" DESTINATION "include")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_toolkit_src})
