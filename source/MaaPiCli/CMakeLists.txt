file(GLOB_RECURSE maa_pi_cli_header ${MAA_PRIVATE_INC}/MaaPiCli/*)

set(maa_pi_cli_core_src
    CLI/interactor.cpp
    CLI/interactor.h
    Impl/Configurator.cpp
    Impl/Parser.cpp
    Impl/Runner.cpp)

add_library(MaaPiCliCore STATIC ${maa_pi_cli_core_src} ${maa_pi_cli_header})

target_include_directories(
    MaaPiCliCore
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaPiCliCore PRIVATE MaaUtils MaaFramework MaaToolkit MaaAgentClient Boost::system ${OpenCV_LIBS})

if(LINUX)
    target_link_libraries(MaaPiCliCore PRIVATE pthread dl)
endif()

add_dependencies(MaaPiCliCore MaaUtils MaaFramework MaaToolkit MaaAgentClient)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_pi_cli_core_src})

if(BUILD_PICLI)
    add_executable(MaaPiCli CLI/main.cpp)

    target_link_libraries(MaaPiCli PRIVATE MaaPiCliCore)

    install(TARGETS MaaPiCli
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
    )

    if(WIN32)
        install(FILES $<TARGET_PDB_FILE:MaaPiCli> DESTINATION symbol OPTIONAL)
    endif()
endif()
