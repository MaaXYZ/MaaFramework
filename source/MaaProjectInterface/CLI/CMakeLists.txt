file(GLOB_RECURSE maa_pi_cli_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_pi_cli_header ${MAA_PRIVATE_INC}/MaaPiCli/*)

add_executable(MaaPiCli ${maa_pi_cli_src} ${maa_pi_cli_header})

target_include_directories(MaaPiCli
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaPiCli ProjectInterface MaaUtils MaaFramework MaaToolkit HeaderOnlyLibraries Boost::system)

add_dependencies(MaaPiCli ProjectInterface MaaUtils MaaFramework MaaToolkit)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_pi_cli_src})

install(TARGETS MaaPiCli
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaPiCli> DESTINATION symbol OPTIONAL)
endif()