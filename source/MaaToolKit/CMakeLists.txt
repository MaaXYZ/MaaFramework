file(GLOB_RECURSE maa_toolkit_src *.h *.hpp *.cpp ../include)

add_library(MaaToolKit SHARED ${maa_toolkit_src})

target_include_directories(MaaToolKit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(MaaToolKit MaaFramework MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS})

target_compile_definitions(MaaToolKit PRIVATE MAA_TOOLKIT_EXPORTS)

add_dependencies(MaaToolKit MaaFramework)

install(TARGETS MaaToolKit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    PUBLIC_HEADER DESTINATION include
    ARCHIVE DESTINATION lib
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${maa_toolkit_src})
