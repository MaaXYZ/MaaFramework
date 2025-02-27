file(GLOB_RECURSE maa_agent_server_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_common_src ../Common/*.h ../Common/*.hpp ../Common/*.cpp)
file(GLOB_RECURSE maa_agent_server_header ${MAA_PUBLIC_INC}/MaaAgentServer/*)

add_library(MaaAgentServer SHARED ${maa_common_src} ${maa_agent_server_src} ${maa_agent_server_header})

target_include_directories(
    MaaAgentServer
    PUBLIC $<BUILD_INTERFACE:${MAA_PUBLIC_INC}> $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_compile_definitions(MaaAgentServer PRIVATE MAA_AGENT_SERVER_EXPORTS)
# 复用 MaaFramework 导出接口
target_compile_definitions(MaaAgentServer PRIVATE MAA_FRAMEWORK_EXPORTS)

target_link_libraries(MaaAgentServer PRIVATE cppzmq-static MaaUtils HeaderOnlyLibraries)

add_dependencies(MaaAgentServer MaaUtils)

install(
    TARGETS MaaAgentServer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaAgentServer> DESTINATION symbol OPTIONAL)
endif()

install(DIRECTORY "${MAA_PUBLIC_INC}/MaaAgentServer" DESTINATION "include")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_agent_server_src})
source_group(PREFIX "Common" TREE ${CMAKE_CURRENT_SOURCE_DIR}/../Common FILES ${maa_common_src})
