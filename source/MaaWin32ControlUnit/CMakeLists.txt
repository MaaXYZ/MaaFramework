file(GLOB_RECURSE maa_win32_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_win32_control_unit_header ${MAA_PUBLIC_INC}/MaaControlUnit/Win32ControlUnitAPI.h ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)

add_control_unit(MaaWin32ControlUnit ${maa_win32_control_unit_src} ${maa_win32_control_unit_header})

target_link_libraries(MaaWin32ControlUnit PRIVATE ZLIB::ZLIB Boost::system)
target_link_libraries(MaaWin32ControlUnit PRIVATE d3d11 dxgi dwmapi runtimeobject winmm)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_win32_control_unit_src})
