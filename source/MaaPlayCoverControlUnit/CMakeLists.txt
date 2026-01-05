file(GLOB_RECURSE maa_playcover_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_playcover_control_unit_header ${MAA_PRIVATE_INC}/ControlUnit/PlayCoverControlUnitAPI.h ${MAA_PRIVATE_INC}/ControlUnit/ControlUnitAPI.h)

add_library(MaaPlayCoverControlUnit SHARED ${maa_playcover_control_unit_src} ${maa_playcover_control_unit_header})

target_include_directories(MaaPlayCoverControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaPlayCoverControlUnit PRIVATE MaaUtils ${OpenCV_LIBS} Boost::system)

target_compile_definitions(MaaPlayCoverControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaPlayCoverControlUnit MaaUtils)

install(
    TARGETS MaaPlayCoverControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_playcover_control_unit_src})
