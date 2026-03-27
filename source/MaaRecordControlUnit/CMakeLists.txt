file(GLOB_RECURSE maa_record_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_record_control_unit_header ${MAA_PUBLIC_INC}/MaaControlUnit/RecordControlUnitAPI.h ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)

add_library(MaaRecordControlUnit SHARED ${maa_record_control_unit_src} ${maa_record_control_unit_header})

target_include_directories(MaaRecordControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaRecordControlUnit MaaUtils ${OpenCV_LIBS})

target_compile_definitions(MaaRecordControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaRecordControlUnit MaaUtils)

install(
    TARGETS MaaRecordControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin

    # ARCHIVE DESTINATION lib
)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaRecordControlUnit> DESTINATION symbol OPTIONAL)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_record_control_unit_src})
