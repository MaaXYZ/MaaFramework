# A single library owns the interface RTTI even when hosts load backends with
# RTLD_LOCAL or supply their own build of MaaUtils.
add_library(MaaControlUnit SHARED ControlUnitCommon/ControlUnitAPI.cpp)
target_compile_definitions(MaaControlUnit PRIVATE MAA_CONTROL_UNIT_TYPES_EXPORTS)
target_compile_features(MaaControlUnit PUBLIC cxx_std_20)
target_include_directories(MaaControlUnit PUBLIC
    $<BUILD_INTERFACE:${MAA_PUBLIC_INC}>
    $<BUILD_INTERFACE:${MAAUTILS_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(MaaControlUnit PUBLIC opencv_core)
# Android builds use the NDK default ANDROID_STL=c++_static. MaaControlUnit does not link MaaUtils, so it pulls in
# its own copy of libc++ and would export it. Control units list MaaControlUnit before MaaUtils in DT_NEEDED and
# would bind std:: symbols to that copy while MaaUtils (Logger, etc.) uses its own, mixing two libc++ runtimes in
# one process. Keep this copy private so std:: symbols keep resolving to MaaUtils as before.
if(ANDROID)
    target_link_options(MaaControlUnit PRIVATE "LINKER:--exclude-libs,libc++_static.a"
                        "LINKER:--exclude-libs,libc++abi.a")
endif()

function(add_control_unit target)
    add_library(${target} SHARED ${ARGN})
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC})
    target_compile_definitions(${target} PRIVATE MAA_CONTROL_UNIT_EXPORTS)
    target_link_libraries(${target} PUBLIC MaaControlUnit PRIVATE MaaUtils ${OpenCV_LIBS})

    install(
        TARGETS ${target}
        EXPORT MaaFrameworkTargets
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib)
    if(WIN32)
        install(FILES $<TARGET_PDB_FILE:${target}> DESTINATION symbol OPTIONAL)
    endif()
endfunction()

if(WITH_ADB_CONTROLLER)
    add_subdirectory(MaaAdbControlUnit)
endif()

if(WITH_IOS_CONTROLLER)
    add_subdirectory(MaaIOSControlUnit)
endif()

if(WITH_WIN32_CONTROLLER)
    add_subdirectory(MaaWin32ControlUnit)
endif()

if(WITH_MACOS_CONTROLLER)
    add_subdirectory(MaaMacOSControlUnit)
endif()

if(WITH_ANDROID_NATIVE_CONTROLLER AND ANDROID)
    add_subdirectory(MaaAndroidNativeControlUnit)
endif()

if(WITH_DBG_CONTROLLER)
    add_subdirectory(MaaDbgControlUnit)
endif()

if(WITH_REPLAY_CONTROLLER)
    add_subdirectory(MaaReplayControlUnit)
endif()

if(WITH_CUSTOM_CONTROLLER)
    add_subdirectory(MaaCustomControlUnit)
endif()

if(WITH_PLAYCOVER_CONTROLLER)
    add_subdirectory(MaaPlayCoverControlUnit)
endif()

if(WITH_GAMEPAD_CONTROLLER)
    add_subdirectory(MaaGamepadControlUnit)
endif()

if(WITH_RECORD_CONTROLLER)
    add_subdirectory(MaaRecordControlUnit)
endif()

if (WITH_LINUX_CONTROLLER)
    add_subdirectory(MaaLinuxControlUnit)
endif ()

add_subdirectory(LibraryHolder)
add_subdirectory(MaaFramework)
add_subdirectory(MaaToolkit)

install(DIRECTORY "${MAA_PUBLIC_INC}/MaaControlUnit" DESTINATION "include")

if(WITH_MAA_AGENT)
    add_subdirectory(MaaAgentClient)
    add_subdirectory(MaaAgentServer)
endif()

add_subdirectory(binding)

# if(ENABLE_CPP20_MODULES)
# add_subdirectory(modules)
# endif()
