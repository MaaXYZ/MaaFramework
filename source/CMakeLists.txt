if(WITH_ADB_CONTROLLER)
    add_subdirectory(MaaAdbControlUnit)
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

if(WITH_WLROOTS_CONTROLLER)
    add_subdirectory(MaaWlRootsControlUnit)
endif()

add_subdirectory(LibraryHolder)
add_subdirectory(MaaFramework)
add_subdirectory(MaaToolkit)

install(DIRECTORY "${MAA_PUBLIC_INC}/MaaControlUnit" DESTINATION "include")

if(BUILD_PICLI)
    add_subdirectory(MaaPiCli)
endif()

if(WITH_MAA_AGENT)
    add_subdirectory(MaaAgentClient)
    add_subdirectory(MaaAgentServer)
endif()

add_subdirectory(binding)

# if(ENABLE_CPP20_MODULES)
# add_subdirectory(modules)
# endif()
