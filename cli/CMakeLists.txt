file(GLOB_RECURSE maa_cli_src *.h *.hpp *.cpp)

add_executable(MaaCli ${maa_cli_src})

target_include_directories(MaaCli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PUBLIC_INC})

target_link_libraries(MaaCli MaaFramework MaaToolkit HeaderOnlyLibraries Boost::system)

add_dependencies(MaaCli MaaFramework MaaToolkit)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_cli_src})

install(
    TARGETS MaaCli
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin)
