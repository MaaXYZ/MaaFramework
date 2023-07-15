# 强行覆盖优化级别，防止产生大量警告
string(REGEX REPLACE "([\\/\\-]O)[23]" "\\11"
  CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

if (MSVC)
    add_compile_options("/utf-8")
    add_compile_options("/MP")
    add_compile_options("/W4;/WX;/Gy;/permissive-;/sdl")
    add_compile_options("/wd4127")  # conditional expression is constant

    set(debug_comp_defs "_DEBUG;MAA_DEBUG")
    add_compile_definitions("$<$<CONFIG:Debug>:${debug_comp_defs}>")

    set(release_comp_options "/Oi;/O1")
    add_compile_options("$<$<CONFIG:Release>:${release_comp_options}>")
    add_compile_options("$<$<CONFIG:ReleaseWithDebInfo>:${release_comp_options}>")

    set(release_link_options "/OPT:REF;/OPT:ICF")
    add_link_options("$<$<CONFIG:Release>:${release_link_options}>")
    add_link_options("$<$<CONFIG:ReleaseWithDebInfo>:${release_link_options}>")

    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options("-Wall;-Werror;-Wextra;-Wpedantic;-Wno-missing-field-initializers")
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
        add_compile_options("-Wno-restrict")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # TODO: Check only perform on mac
    set(MAC_CXX_EXPERIMENTAL_PATH "/opt/local/libexec/llvm-16/lib" CACHE STRING "path of libc++experimental.a")
    set(CMAKE_MACOSX_RPATH 1)
    list(APPEND CMAKE_BUILD_RPATH "${MAC_CXX_EXPERIMENTAL_PATH}")

    add_compile_options("-fexperimental-library")
    add_compile_options("-Wno-deprecated-declarations")
    add_link_options("-L${MAC_CXX_EXPERIMENTAL_PATH}")
    add_link_options("-lc++experimental")
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
