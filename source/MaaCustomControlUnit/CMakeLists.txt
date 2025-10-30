file(GLOB_RECURSE maa_custom_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_custom_control_unit_header ${MAA_PRIVATE_INC}/ControlUnit/CustomControlUnitAPI.h ${MAA_PRIVATE_INC}/ControlUnit/ControlUnitAPI.h)

add_library(MaaCustomControlUnit SHARED ${maa_custom_control_unit_src} ${maa_custom_control_unit_header})

target_include_directories(MaaCustomControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaCustomControlUnit MaaUtils ${OpenCV_LIBS})

target_compile_definitions(MaaCustomControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaCustomControlUnit MaaUtils)

install(
    TARGETS MaaCustomControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin

    # ARCHIVE DESTINATION lib
)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaCustomControlUnit> DESTINATION symbol OPTIONAL)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_custom_control_unit_src})
