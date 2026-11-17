file(GLOB_RECURSE maa_gamepad_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_gamepad_control_unit_header
    ${MAA_PUBLIC_INC}/MaaControlUnit/GamepadControlUnitAPI.h
    ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)

add_control_unit(MaaGamepadControlUnit ${maa_gamepad_control_unit_src} ${maa_gamepad_control_unit_header})

target_include_directories(MaaGamepadControlUnit PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/include)

target_link_libraries(MaaGamepadControlUnit PRIVATE Boost::system)

# Depend on Win32ControlUnit for screencap functionality (runtime dependency, loaded dynamically)
if(WITH_WIN32_CONTROLLER)
    add_dependencies(MaaGamepadControlUnit MaaWin32ControlUnit)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_gamepad_control_unit_src})
