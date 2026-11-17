set(XDB_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(XDB_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(XDB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(XDB_INSTALL OFF CACHE BOOL "" FORCE)
# Allow xdb to download its pinned nlohmann_json version when no suitable package is found.
set(XDB_FETCH_DEPENDENCIES ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/XDeviceBridge ${CMAKE_BINARY_DIR}/3rdparty/XDeviceBridge)

file(GLOB_RECURSE maa_ios_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_ios_control_unit_header ${MAA_PUBLIC_INC}/MaaControlUnit/IOSControlUnitAPI.h ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)

add_control_unit(MaaIOSControlUnit ${maa_ios_control_unit_src} ${maa_ios_control_unit_header})

target_link_libraries(MaaIOSControlUnit PRIVATE xdb_client)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_ios_control_unit_src})

install(
    TARGETS xdb_client
    EXPORT MaaFrameworkTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin NAMELINK_SKIP
    ARCHIVE DESTINATION lib)

install(FILES ${CMAKE_SOURCE_DIR}/3rdparty/XDeviceBridge/LICENSE ${CMAKE_SOURCE_DIR}/3rdparty/XDeviceBridge/COPYING.GPLv3 DESTINATION share/licenses/XDeviceBridge)
