add_library(MaaWin32GetCursorPosHook SHARED GetCursorPosHookDll.cpp)

target_include_directories(MaaWin32GetCursorPosHook PRIVATE ${CMAKE_SOURCE_DIR}/source/MaaWin32ControlUnit/Input)

install(
    TARGETS MaaWin32GetCursorPosHook
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaWin32GetCursorPosHook> DESTINATION symbol OPTIONAL)
endif()
