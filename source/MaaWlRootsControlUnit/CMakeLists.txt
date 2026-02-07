file(GLOB_RECURSE maa_wlroots_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_wlroots_control_unit_header ${MAA_PRIVATE_INC}/ControlUnit/WlRootsControlUnitAPI.h ${MAA_PRIVATE_INC}/ControlUnit/ControlUnitAPI.h)

add_library(MaaWlRootsControlUnit SHARED ${maa_wlroots_control_unit_src} ${maa_wlroots_control_unit_header})

target_include_directories(MaaWlRootsControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

find_library(Wayland_LIBS wayland-client)
find_library(FFI_LIBS ffi)
target_link_libraries(MaaWlRootsControlUnit PRIVATE MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS} Boost::system ${Wayland_LIBS} ${FFI_LIBS})

target_compile_definitions(MaaWlRootsControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaWlRootsControlUnit MaaUtils)

install(
    TARGETS MaaWlRootsControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_wlroots_control_unit_src})
