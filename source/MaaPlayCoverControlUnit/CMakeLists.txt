file(GLOB_RECURSE maa_playcover_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_playcover_control_unit_header ${MAA_PUBLIC_INC}/MaaControlUnit/PlayCoverControlUnitAPI.h ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)

set(maa_macos_app_lifecycle_src
    ${CMAKE_CURRENT_SOURCE_DIR}/../ControlUnitCommon/MacOS/MacOSAppLifecycle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../ControlUnitCommon/MacOS/MacOSAppLifecycle.mm)

add_control_unit(MaaPlayCoverControlUnit ${maa_playcover_control_unit_src} ${maa_playcover_control_unit_header} ${maa_macos_app_lifecycle_src})

target_include_directories(MaaPlayCoverControlUnit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ControlUnitCommon)

target_link_libraries(MaaPlayCoverControlUnit PRIVATE Boost::system)

find_library(APP_KIT AppKit REQUIRED)
find_library(APPLICATION_SERVICES ApplicationServices REQUIRED)
target_link_libraries(MaaPlayCoverControlUnit PRIVATE ${APP_KIT} ${APPLICATION_SERVICES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_playcover_control_unit_src})

source_group(PREFIX "ControlUnitCommon" TREE ${CMAKE_CURRENT_SOURCE_DIR}/../ControlUnitCommon FILES ${maa_macos_app_lifecycle_src})
