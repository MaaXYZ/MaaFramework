file(GLOB_RECURSE maa_pi_cli_next_src *.h *.hpp *.cpp)

add_executable(MaaPiCliNext ${maa_pi_cli_next_src})

target_include_directories(MaaPiCliNext PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaPiCliNext MaaUtils MaaFramework MaaToolkit HeaderOnlyLibraries Boost::system)

add_dependencies(MaaPiCliNext MaaUtils MaaFramework MaaToolkit)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_pi_cli_next_src})

install(
    TARGETS MaaPiCliNext
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin)
