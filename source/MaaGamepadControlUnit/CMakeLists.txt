file(GLOB_RECURSE maa_gamepad_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_gamepad_control_unit_header
    ${MAA_PRIVATE_INC}/ControlUnit/GamepadControlUnitAPI.h
    ${MAA_PRIVATE_INC}/ControlUnit/ControlUnitAPI.h
    ${MAA_PRIVATE_INC}/ControlUnit/Win32ControlUnitAPI.h)

add_library(MaaGamepadControlUnit SHARED ${maa_gamepad_control_unit_src} ${maa_gamepad_control_unit_header})

target_include_directories(MaaGamepadControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaGamepadControlUnit PRIVATE HeaderOnlyLibraries MaaUtils)

target_compile_definitions(MaaGamepadControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaGamepadControlUnit MaaUtils)

install(
    TARGETS MaaGamepadControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaGamepadControlUnit> DESTINATION symbol OPTIONAL)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_gamepad_control_unit_src})
