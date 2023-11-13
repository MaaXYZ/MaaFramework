file(GLOB_RECURSE library_holder_src *.h *.hpp *.cpp)
file(GLOB_RECURSE library_holder_header ../include/LibraryHolder/*)

add_library(LibraryHolder STATIC ${library_holder_src} ${library_holder_header})
set_target_properties(LibraryHolder PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(LibraryHolder 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(LibraryHolder MaaUtils HeaderOnlyLibraries Boost::system)

add_dependencies(LibraryHolder MaaUtils)
if (WITH_ADB_CONTROLLER)
    add_dependencies(LibraryHolder MaaAdbControlUnit)
endif()
if (WITH_WIN32_CONTROLLER)
    add_dependencies(LibraryHolder MaaWin32ControlUnit)
endif()
if(WITH_THRIFT_CONTROLLER)
    add_dependencies(LibraryHolder MaaThriftControlUnit)
endif()
if (WITH_DBG_CONTROLLER)
    add_dependencies(LibraryHolder MaaDbgControlUnit)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${library_holder_src})
