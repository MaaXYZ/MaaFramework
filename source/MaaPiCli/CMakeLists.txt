if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.28)
    project(MaaPiCli LANGUAGES CXX)

    option(BUILD_PICLI "build maa picli" ON)

    find_package(MaaFramework CONFIG REQUIRED)

    set(MAA_PI_CLI_FRAMEWORK_TARGETS
        MaaFramework::MaaFramework MaaFramework::MaaToolkit MaaFramework::MaaAgentClient
        MaaFramework::MaaUtils)
else()
    set(MAA_PI_CLI_FRAMEWORK_TARGETS MaaFramework MaaToolkit MaaAgentClient MaaUtils)
endif()

file(GLOB_RECURSE maa_pi_cli_header include/ProjectInterface/*.h)

set(maa_pi_cli_core_src
    CLI/interactor.cpp
    CLI/interactor.h
    Impl/Configurator.cpp
    Impl/Parser.cpp
    Impl/Runner.cpp)

add_library(MaaPiCliCore STATIC ${maa_pi_cli_core_src} ${maa_pi_cli_header})

target_include_directories(
    MaaPiCliCore
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    target_include_directories(MaaPiCliCore PUBLIC ${MAA_PUBLIC_INC})
endif()

target_compile_features(MaaPiCliCore PUBLIC cxx_std_20)
target_link_libraries(MaaPiCliCore PUBLIC ${MAA_PI_CLI_FRAMEWORK_TARGETS})

if(LINUX)
    target_link_libraries(MaaPiCliCore PRIVATE pthread dl)
endif()

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
