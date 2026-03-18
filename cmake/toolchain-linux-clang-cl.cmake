# cmake/toolchain-linux-clang-cl.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

# Specify the compiler
set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_RC_COMPILER llvm-rc)
set(CMAKE_MT llvm-mt)
set(CMAKE_AR llvm-lib)
set(CMAKE_LINKER lld-link)

# Where is the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# XWIN_CACHE_DIR setup
if(NOT DEFINED XWIN_CACHE_DIR)
    if(DEFINED ENV{XWIN_CACHE_DIR})
        set(XWIN_CACHE_DIR $ENV{XWIN_CACHE_DIR})
    else()
        message(FATAL_ERROR "XWIN_CACHE_DIR is not defined.")
    endif()
endif()

set(MSVC_CRT_DIR "${XWIN_CACHE_DIR}/crt")
set(MSVC_SDK_DIR "${XWIN_CACHE_DIR}/sdk")

# =============================================================================
# Fix: Case sensitivity issues (Symlinks)
# =============================================================================
set(_MSVC_UM_DIR "${MSVC_SDK_DIR}/lib/um/x86_64")

if(EXISTS "${_MSVC_UM_DIR}/cfgmgr32.lib" AND NOT EXISTS "${_MSVC_UM_DIR}/Cfgmgr32.lib")
    message(STATUS "Toolchain: Creating symlink for Cfgmgr32.lib...")
    file(CREATE_LINK "${_MSVC_UM_DIR}/cfgmgr32.lib" "${_MSVC_UM_DIR}/Cfgmgr32.lib" SYMBOLIC)
endif()

set(_MSVC_CPPWINRT_DIR "${MSVC_SDK_DIR}/include/cppwinrt")

if(EXISTS "${MSVC_SDK_DIR}/include/CppWinRT" AND NOT EXISTS "${_MSVC_CPPWINRT_DIR}")
    file(CREATE_LINK "${MSVC_SDK_DIR}/include/CppWinRT" "${_MSVC_CPPWINRT_DIR}" SYMBOLIC)
endif()

if(EXISTS "${_MSVC_CPPWINRT_DIR}/WinRT" AND NOT EXISTS "${_MSVC_CPPWINRT_DIR}/winrt")
    file(CREATE_LINK "${_MSVC_CPPWINRT_DIR}/WinRT" "${_MSVC_CPPWINRT_DIR}/winrt" SYMBOLIC)
endif()

set(_TARGET_HEADER_DIR "${_MSVC_CPPWINRT_DIR}/winrt")

if(EXISTS "${_TARGET_HEADER_DIR}")
    if(EXISTS "${_TARGET_HEADER_DIR}/windows.graphics.capture.h" AND NOT EXISTS "${_TARGET_HEADER_DIR}/Windows.Graphics.Capture.h")
        file(CREATE_LINK "${_TARGET_HEADER_DIR}/windows.graphics.capture.h" "${_TARGET_HEADER_DIR}/Windows.Graphics.Capture.h" SYMBOLIC)
    endif()

    if(EXISTS "${_TARGET_HEADER_DIR}/windows.graphics.directx.direct3d11.h" AND NOT EXISTS "${_TARGET_HEADER_DIR}/Windows.Graphics.DirectX.Direct3D11.h")
        file(CREATE_LINK "${_TARGET_HEADER_DIR}/windows.graphics.directx.direct3d11.h" "${_TARGET_HEADER_DIR}/Windows.Graphics.DirectX.Direct3D11.h" SYMBOLIC)
    endif()
endif()

# =============================================================================
# Compile Flags (Include Paths & Warnings)
# =============================================================================
set(_MAA_SYS_INCLUDES
    "/imsvc \"${MSVC_CRT_DIR}/include\""
    "/imsvc \"${MSVC_SDK_DIR}/include/ucrt\""
    "/imsvc \"${MSVC_SDK_DIR}/include/shared\""
    "/imsvc \"${MSVC_SDK_DIR}/include/um\""
    "/imsvc \"${MSVC_SDK_DIR}/include/winrt\""
    "/imsvc \"${MSVC_SDK_DIR}/include/cppwinrt\""
)
string(REPLACE ";" " " _MAA_SYS_INCLUDES_STR "${_MAA_SYS_INCLUDES}")

set(_MAA_SUPPRESS_FLAGS "\
/clang:-Wno-everything \
/clang:-Wno-unused-command-line-argument \
/clang:-Wno-sign-compare \
/clang:-Wno-error=unused-command-line-argument \
/clang:-Wno-missing-field-initializers \
/clang:-Wno-missing-braces \
/clang:-Wno-#pragma-messages \
/clang:-Wno-macro-redefined \
/clang:-Wno-double-promotion \
/clang:-Wno-float-equal \
/clang:-Wno-cast-qual \
/clang:-Wno-format-nonliteral \
/clang:-Wno-declaration-after-statement \
/clang:-Wno-implicit-int-float-conversion \
/clang:-Wno-bad-function-cast \
/clang:-Wno-shadow \
/clang:-Wno-switch-default \
/clang:-Wno-cast-align \
/clang:-Wno-undef \
/clang:-Wno-duplicate-enum \
/clang:-Wno-unused-macros \
/clang:-Wno-missing-prototypes \
/clang:-Wno-implicit-float-conversion \
/clang:-Wno-missing-variable-declarations \
/clang:-Wno-conditional-uninitialized \
/clang:-Wno-float-conversion \
/clang:-Wno-covered-switch-default \
/clang:-Wno-unreachable-code-break \
/clang:-Wno-unreachable-code-return \
/clang:-Wno-unreachable-code \
/clang:-Wno-switch-enum \
/clang:-Wno-unused-parameter \
/clang:-Wno-unused-variable \
/clang:-Wno-unused-function \
/clang:-Wno-unused-label \
/clang:-Wno-unused-value \
/clang:-Wno-implicit-fallthrough \
/clang:-Wno-strict-prototypes \
/clang:-Wno-shorten-64-to-32 \
/clang:-Wno-sign-conversion \
/clang:-Wno-int-conversion \
/clang:-Wno-incompatible-pointer-types \
/clang:-Wno-newline-eof \
/clang:-Wno-deprecated-declarations \
/clang:-Wno-shift-op-parentheses \
/clang:-Wno-bitwise-op-parentheses \
/clang:-Wno-logical-op-parentheses \
/clang:-Wno-parentheses-equality \
/clang:-Wno-microsoft-enum-value \
/clang:-Wno-microsoft-include \
/clang:-Wno-unknown-pragmas \
/clang:-Wno-used-but-marked-unused \
/clang:-Wno-cast-function-type-strict \
/clang:-Wno-comma \
")

# Set Initial Flags
set(CMAKE_C_FLAGS_INIT "${_MAA_SYS_INCLUDES_STR} ${_MAA_SUPPRESS_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${_MAA_SYS_INCLUDES_STR} /EHsc /arch:AVX ${_MAA_SUPPRESS_FLAGS}")

# =============================================================================
# Linker Configuration
# =============================================================================

# 1. Force add library search paths
add_link_options(
    "/libpath:${MSVC_CRT_DIR}/lib/x86_64"
    "/libpath:${MSVC_SDK_DIR}/lib/ucrt/x86_64"
    "/libpath:${MSVC_SDK_DIR}/lib/um/x86_64"
)

# 2. Resolve -lpthreads issue
set(CMAKE_THREAD_LIBS_INIT "")
set(CMAKE_HAVE_LIBC_PTHREAD 1)
set(CMAKE_USE_WIN32_THREADS_INIT 1)

# 3. Global link standard libraries
link_libraries(
    kernel32 user32 gdi32 winspool shell32 ole32 oleaut32 uuid comdlg32 advapi32
    msvcrt oldnames msvcprt
    delayimp
)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

add_compile_definitions(
    _AMD64_
    _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# 4. Ensure warnings are ignored during compilation
add_compile_options(
    -Wno-unused-command-line-argument
    -Wno-sign-compare
    -Wno-error=unused-command-line-argument
    -Wno-missing-field-initializers
    -Wno-missing-braces
    "-Wno-#pragma-messages"
    -Wno-macro-redefined
    -Wno-double-promotion
    -Wno-float-equal
    -Wno-cast-qual
    -Wno-format-nonliteral
    -Wno-declaration-after-statement
    -Wno-implicit-int-float-conversion
    -Wno-bad-function-cast
    -Wno-shadow
    -Wno-switch-default
    -Wno-cast-align
    -Wno-undef
    -Wno-duplicate-enum
    -Wno-unused-macros
    -Wno-missing-prototypes
    -Wno-implicit-float-conversion
    -Wno-missing-variable-declarations
    -Wno-conditional-uninitialized
    -Wno-float-conversion
    -Wno-covered-switch-default
    -Wno-unreachable-code-break
    -Wno-unreachable-code-return
    -Wno-unreachable-code
    -Wno-switch-enum
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-unused-label
    -Wno-unused-value
    -Wno-implicit-fallthrough
    -Wno-strict-prototypes
    -Wno-shorten-64-to-32
    -Wno-sign-conversion
    -Wno-int-conversion
    -Wno-incompatible-pointer-types
    -Wno-newline-eof
    -Wno-deprecated-declarations
    -Wno-shift-op-parentheses
    -Wno-bitwise-op-parentheses
    -Wno-logical-op-parentheses
    -Wno-parentheses-equality
    -Wno-microsoft-enum-value
    -Wno-microsoft-include
    -Wno-unknown-pragmas
    -Wno-used-but-marked-unused
    -Wno-cast-function-type-strict
    -Wno-comma
)