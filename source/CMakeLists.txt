add_subdirectory(MaaUtils)

if(WITH_ADB_CONTROLLER)
    add_subdirectory(MaaAdbControlUnit)
endif()

if(WITH_WIN32_CONTROLLER)
    add_subdirectory(MaaWin32ControlUnit)
endif()

if(WITH_DBG_CONTROLLER)
    add_subdirectory(MaaDbgControlUnit)
endif()

add_subdirectory(LibraryHolder)
add_subdirectory(MaaFramework)
add_subdirectory(MaaToolkit)
add_subdirectory(MaaProjectInterface)
add_subdirectory(MaaAgentClient)
add_subdirectory(MaaAgentServer)

add_subdirectory(binding)
