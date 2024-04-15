find_program(NODE_EXE NAMES node nodejs)
if(NODE_EXE AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/LHG/README.md)
    set(LHG_EMBEDDED
        ON
        CACHE BOOL "make lhg work in embedded mode")

    add_subdirectory(LHG)

    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    lhggenerate(LHGInfo ${CMAKE_CURRENT_SOURCE_DIR}/interface.json ${CMAKE_CURRENT_SOURCE_DIR}/config.json
                ${CMAKE_CURRENT_SOURCE_DIR}/source/info.hpp)

    file(GLOB_RECURSE maa_http_src source/*.cpp source/*.h source/*.hpp)

    add_executable(MaaHttp ${maa_http_src})
    target_include_directories(MaaHttp PRIVATE ./../include)
    target_link_libraries(MaaHttp LHGSupport HeaderOnlyLibraries Boost::system MaaFramework MaaToolkit)
    add_dependencies(MaaHttp LHGInfo)

    if(WIN32)
        target_compile_definitions(MaaHttp PRIVATE _WIN32_WINNT=0x0A00)
        target_compile_options(MaaHttp PRIVATE /bigobj)
    endif()

    install(
        TARGETS MaaHttp
        EXPORT MaaFrameworkTargets
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib)

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_http_src})

else()
    message(STATUS "NodeJS not found or LHG submodule not exists, Ignoring BUILD_HTTP_CLI")
endif()
