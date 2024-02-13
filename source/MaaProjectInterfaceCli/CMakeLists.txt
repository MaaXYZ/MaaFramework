file(GLOB_RECURSE maa_pi_cli_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_pi_cli_header ../include/MaaPiCli/*)

add_executable(MaaPiCli ${maa_pi_cli_src} ${maa_pi_cli_header})

target_include_directories(MaaPiCli 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(MaaPiCli ProjectInterface MaaUtils HeaderOnlyLibraries Boost::system)

add_dependencies(MaaPiCli ProjectInterface MaaUtils MaaFramework MaaToolkit)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_pi_cli_src})
