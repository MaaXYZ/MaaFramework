file(GLOB_RECURSE library_holder_src *.h *.hpp *.cpp)
file(GLOB_RECURSE library_holder_header ${MAA_PRIVATE_INC}/LibraryHolder/*)

add_library(LibraryHolder STATIC ${library_holder_src} ${library_holder_header})
set_target_properties(LibraryHolder PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(LibraryHolder
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(LibraryHolder MaaUtils Boost::system ${OpenCV_LIBS})

add_dependencies(LibraryHolder MaaUtils)

if(WITH_ADB_CONTROLLER)
    add_dependencies(LibraryHolder MaaAdbControlUnit)
endif()

if(WITH_WIN32_CONTROLLER)
    add_dependencies(LibraryHolder MaaWin32ControlUnit)
endif()

if(WITH_PLAYCOVER_CONTROLLER)
    add_dependencies(LibraryHolder MaaPlayCoverControlUnit)
endif()

if(WITH_DBG_CONTROLLER)
    add_dependencies(LibraryHolder MaaDbgControlUnit)
endif()

if(WITH_CUSTOM_CONTROLLER)
    add_dependencies(LibraryHolder MaaCustomControlUnit)
endif()

if(WITH_GAMEPAD_CONTROLLER)
    add_dependencies(LibraryHolder MaaGamepadControlUnit)
endif()

if(WITH_PROXY_CONTROLLER)
    add_dependencies(LibraryHolder MaaProxyControlUnit)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${library_holder_src})
