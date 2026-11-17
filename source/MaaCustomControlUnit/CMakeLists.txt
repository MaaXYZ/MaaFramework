file(GLOB_RECURSE maa_custom_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_custom_control_unit_header ${MAA_PUBLIC_INC}/MaaControlUnit/CustomControlUnitAPI.h ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)

add_control_unit(MaaCustomControlUnit ${maa_custom_control_unit_src} ${maa_custom_control_unit_header})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_custom_control_unit_src})
