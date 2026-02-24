file(GLOB_RECURSE maa_proxy_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_proxy_control_unit_header ${MAA_PRIVATE_INC}/ControlUnit/ProxyControlUnitAPI.h ${MAA_PRIVATE_INC}/ControlUnit/ControlUnitAPI.h)

add_library(MaaProxyControlUnit SHARED ${maa_proxy_control_unit_src} ${maa_proxy_control_unit_header})

target_include_directories(MaaProxyControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaProxyControlUnit MaaUtils ${OpenCV_LIBS})

target_compile_definitions(MaaProxyControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaProxyControlUnit MaaUtils)

install(
    TARGETS MaaProxyControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin

    # ARCHIVE DESTINATION lib
)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaProxyControlUnit> DESTINATION symbol OPTIONAL)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_proxy_control_unit_src})
