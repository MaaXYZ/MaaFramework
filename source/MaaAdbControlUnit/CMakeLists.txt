file(GLOB_RECURSE maa_adb_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_adb_control_unit_header ${MAA_PRIVATE_INC}/ControlUnit/AdbControlUnitAPI.h ${MAA_PRIVATE_INC}/ControlUnit/ControlUnitAPI.h)

add_library(MaaAdbControlUnit SHARED ${maa_adb_control_unit_src} ${maa_adb_control_unit_header})

target_include_directories(MaaAdbControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaAdbControlUnit MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS} ZLIB::ZLIB EmulatorExtras)

if(WIN32)
    target_link_libraries(MaaAdbControlUnit ws2_32)
endif()

target_compile_definitions(MaaAdbControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaAdbControlUnit MaaUtils)

install(
    TARGETS MaaAdbControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin

    # ARCHIVE DESTINATION lib
)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaAdbControlUnit> DESTINATION symbol OPTIONAL)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_adb_control_unit_src})
