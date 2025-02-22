file(GLOB_RECURSE maa_agent_client_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_agent_client_header ${MAA_PUBLIC_INC}/MaaAgentClient/*)

add_library(MaaAgentClient SHARED ${maa_agent_client_src} ${maa_agent_client_header})

target_include_directories(
    MaaAgentClient
    PUBLIC $<BUILD_INTERFACE:${MAA_PUBLIC_INC}> $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_compile_definitions(MaaAgentClient PRIVATE MAA_AGENT_CLIENT_EXPORTS)

target_link_libraries(MaaAgentClient PRIVATE cppzmq-static MaaUtils MaaFramework LibraryHolder HeaderOnlyLibraries)

add_dependencies(MaaAgentClient MaaUtils MaaFramework)

install(
    TARGETS MaaAgentClient
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaAgentClient> DESTINATION symbol OPTIONAL)
endif()

install(DIRECTORY "${MAA_PUBLIC_INC}/MaaAgentClient" DESTINATION "include")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_agent_client_src})
