file(GLOB_RECURSE maa_gamescope_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_gamescope_control_unit_header ${MAA_PUBLIC_INC}/MaaControlUnit/GamescopeControlUnitAPI.h ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)

add_library(MaaGamescopeControlUnit SHARED ${maa_gamescope_control_unit_src} ${maa_gamescope_control_unit_header})

target_include_directories(MaaGamescopeControlUnit
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

find_path(PipeWire_INCLUDE_DIR pipewire/pipewire.h PATH_SUFFIXES pipewire-0.3)
find_path(Spa_INCLUDE_DIR spa/param/video/format-utils.h PATH_SUFFIXES spa-0.2)
find_library(PipeWire_LIBS pipewire-0.3)
find_path(LibEI_INCLUDE_DIR libei.h PATH_SUFFIXES libei-1.0)
find_library(LibEI_LIBS ei)

target_include_directories(MaaGamescopeControlUnit SYSTEM PRIVATE ${PipeWire_INCLUDE_DIR} ${Spa_INCLUDE_DIR} ${LibEI_INCLUDE_DIR})
target_link_libraries(MaaGamescopeControlUnit PRIVATE MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS} ${PipeWire_LIBS} ${LibEI_LIBS})

target_compile_definitions(MaaGamescopeControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaGamescopeControlUnit MaaUtils)

install(
        TARGETS MaaGamescopeControlUnit
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_gamescope_control_unit_src})
