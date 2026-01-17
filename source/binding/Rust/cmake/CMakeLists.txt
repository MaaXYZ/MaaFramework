cmake_minimum_required(VERSION 3.12)

project(MAA_FW_PROBE)

find_package(MaaFramework REQUIRED CONFIGS MaaFramework.cmake)

if(WIN32)
get_target_property(MaaFramework_LIBRARIES MaaFramework::MaaFramework IMPORTED_IMPLIB_RELEASE)
get_target_property(MaaToolkit_LIBRARIES MaaFramework::MaaToolkit IMPORTED_IMPLIB_RELEASE)
endif(WIN32)
if(UNIX)
get_target_property(MaaFramework_LIBRARIES MaaFramework::MaaFramework IMPORTED_LOCATION_RELEASE)
get_target_property(MaaToolkit_LIBRARIES MaaFramework::MaaToolkit IMPORTED_LOCATION_RELEASE)
endif(UNIX)

get_target_property(MaaFramework_INCLUDE_DIRS MaaFramework::MaaFramework INTERFACE_INCLUDE_DIRECTORIES)

add_executable(MAA_FW_PROBE maa_framework_probe.cpp)

target_link_libraries(MAA_FW_PROBE MaaFramework)

message("IncludeDir=${MaaFramework_INCLUDE_DIRS}")
message("MaaFrameworkLibraries=${MaaFramework_LIBRARIES}")
message("MaaToolkitLibraries=${MaaToolkit_LIBRARIES}")
