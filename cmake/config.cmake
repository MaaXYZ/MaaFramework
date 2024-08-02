set(debug_comp_defs "_DEBUG;MAA_DEBUG")
add_compile_definitions("$<$<CONFIG:Debug>:${debug_comp_defs}>")

set(rel_debug_comp_defs "MAA_DEBUG")
add_compile_definitions("$<$<CONFIG:DebWithRelDeps>:${rel_debug_comp_defs}>")

if(MSVC)
    add_compile_options("/utf-8")
    add_compile_options("/MP")
    add_compile_options("/W4;/WX;/Gy;/permissive-;/sdl")
    add_compile_options("/wd4127") # conditional expression is constant
    add_compile_options("/wd4251") # export dll with templates

    # https://github.com/actions/runner-images/issues/10004
    # https://github.com/microsoft/STL/releases/tag/vs-2022-17.10
    add_compile_definitions("_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR")

    set(rel_debug_comp_options "/Od")
    add_compile_options("$<$<CONFIG:DebWithRelDeps>:${rel_debug_comp_options}>")

    set(release_link_options "/OPT:REF;/OPT:ICF")
    add_link_options("$<$<CONFIG:Release>:${release_link_options}>")

    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
else()
    add_compile_options("-Wall;-Werror;-Wextra;-Wpedantic;-Wno-missing-field-initializers")

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
        add_compile_options("-Wno-restrict")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 18)
        set(MAC_CLANG_FIX_LINK
            "/opt/local/libexec/llvm-18/lib/libc++" # mac port
            "/usr/local/opt/llvm/lib/c++" # brew mac 13.*
            "/opt/homebrew/opt/llvm/lib/c++" # brew mac 14.*
        )

        foreach(DIR ${MAC_CLANG_FIX_LINK})
            if(IS_DIRECTORY ${DIR})
                add_link_options("-L${DIR}")
            endif()
        endforeach()
    endif()

    set(rel_debug_comp_options "-O0")
    add_compile_options("$<$<CONFIG:DebWithRelDeps>:${rel_debug_comp_options}>")
endif()

if(APPLE)
    set(CMAKE_INSTALL_RPATH "@loader_path;@executable_path")
elseif(UNIX)
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
