# Native Ubuntu integration only. Retain MaaDeps' libc++ ABI, while using
# the runner's glibc headers and libraries together with its OpenSSL/FFmpeg/curl.
# The release toolchain continues to use its original portable glibc sysroot.
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" OR
   NOT CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    message(FATAL_ERROR "The iOS native CI toolchain requires a Linux x64 host")
endif()

get_filename_component(maa_ios_repository "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(maa_ios_deps "${maa_ios_repository}/source/MaaUtils/MaaDeps")
set(maa_ios_sysroot "${maa_ios_deps}/x-tools/x86_64-linux-gnu/x86_64-linux-gnu/sysroot")
set(maa_ios_runtime "${maa_ios_repository}/build/ios-native-deps/lib")
file(MAKE_DIRECTORY "${maa_ios_runtime}")
# Copy only C++ runtimes, so the old sysroot's libc cannot enter the link or RPATH.
foreach(maa_ios_library IN ITEMS libc++ libc++abi libunwind)
    file(GLOB maa_ios_library_files "${maa_ios_sysroot}/usr/lib/${maa_ios_library}.so*")
    if(NOT maa_ios_library_files)
        message(FATAL_ERROR "MaaDeps runtime not found: ${maa_ios_library}")
    endif()
    file(COPY ${maa_ios_library_files} DESTINATION "${maa_ios_runtime}" FOLLOW_SYMLINK_CHAIN)
endforeach()

set(CMAKE_C_COMPILER "${maa_ios_deps}/x-tools/llvm/bin/clang")
set(CMAKE_CXX_COMPILER "${maa_ios_deps}/x-tools/llvm/bin/clang++")
set(CMAKE_C_COMPILER_TARGET x86_64-linux-gnu)
set(CMAKE_CXX_COMPILER_TARGET x86_64-linux-gnu)
set(CMAKE_C_FLAGS_INIT "-fPIC")
# The explicit headers select libc++; -stdlib is needed only for linking.
# With -nostdinc++, Clang diagnoses a compile-time -stdlib as unused.
set(CMAKE_CXX_FLAGS_INIT
    "-fPIC -nostdinc++ -isystem \"${maa_ios_sysroot}/usr/include/c++/v1\"")
set(maa_ios_link_flags
    "-fuse-ld=lld -stdlib=libc++ -L\"${maa_ios_runtime}\" -Wl,-rpath,${maa_ios_runtime} -Wl,-rpath-link,${maa_ios_runtime}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${maa_ios_link_flags}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${maa_ios_link_flags}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${maa_ios_link_flags}")
set(CMAKE_LINKER_TYPE LLD)
