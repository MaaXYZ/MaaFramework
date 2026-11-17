file(GLOB_RECURSE maa_adb_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_adb_control_unit_header ${MAA_PUBLIC_INC}/MaaControlUnit/AdbControlUnitAPI.h ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)

add_control_unit(MaaAdbControlUnit ${maa_adb_control_unit_src} ${maa_adb_control_unit_header})

target_link_libraries(MaaAdbControlUnit PRIVATE HeaderOnlyLibraries ZLIB::ZLIB EmulatorExtras ${CMAKE_DL_LIBS})

if(WIN32)
    target_link_libraries(MaaAdbControlUnit PRIVATE ws2_32)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_adb_control_unit_src})
