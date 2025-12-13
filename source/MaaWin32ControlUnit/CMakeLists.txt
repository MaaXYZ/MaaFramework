file(GLOB_RECURSE maa_win32_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_win32_control_unit_header ${MAA_PRIVATE_INC}/ControlUnit/Win32ControlUnitAPI.h ${MAA_PRIVATE_INC}/ControlUnit/ControlUnitAPI.h)

add_library(MaaWin32ControlUnit SHARED ${maa_win32_control_unit_src} ${maa_win32_control_unit_header})

target_include_directories(MaaWin32ControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaWin32ControlUnit PRIVATE HeaderOnlyLibraries MaaUtils ${OpenCV_LIBS} ZLIB::ZLIB Boost::system)
target_link_libraries(MaaWin32ControlUnit PRIVATE d3d11 dxgi runtimeobject)

target_compile_definitions(MaaWin32ControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaWin32ControlUnit MaaUtils)

install(
    TARGETS MaaWin32ControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin

    # ARCHIVE DESTINATION lib
)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaWin32ControlUnit> DESTINATION symbol OPTIONAL)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_win32_control_unit_src})
