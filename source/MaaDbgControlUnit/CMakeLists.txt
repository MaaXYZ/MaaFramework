file(GLOB_RECURSE maa_dbg_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_dbg_control_unit_header ${MAA_PRIVATE_INC}/ControlUnit/DbgControlUnitAPI.h ${MAA_PRIVATE_INC}/ControlUnit/ControlUnitAPI.h)

add_library(MaaDbgControlUnit SHARED ${maa_dbg_control_unit_src} ${maa_dbg_control_unit_header})

target_include_directories(MaaDbgControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaDbgControlUnit MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS})

target_compile_definitions(MaaDbgControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaDbgControlUnit MaaUtils)

install(
    TARGETS MaaDbgControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin

    # ARCHIVE DESTINATION lib
)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaDbgControlUnit> DESTINATION symbol OPTIONAL)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_dbg_control_unit_src})
