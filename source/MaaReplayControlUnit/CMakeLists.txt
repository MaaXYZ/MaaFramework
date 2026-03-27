file(GLOB_RECURSE maa_replay_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_replay_control_unit_header ${MAA_PUBLIC_INC}/MaaControlUnit/ReplayControlUnitAPI.h ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)

add_library(MaaReplayControlUnit SHARED ${maa_replay_control_unit_src} ${maa_replay_control_unit_header})

target_include_directories(MaaReplayControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaReplayControlUnit MaaUtils ${OpenCV_LIBS})

target_compile_definitions(MaaReplayControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaReplayControlUnit MaaUtils)

install(
    TARGETS MaaReplayControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin

    # ARCHIVE DESTINATION lib
)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaReplayControlUnit> DESTINATION symbol OPTIONAL)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_replay_control_unit_src})
