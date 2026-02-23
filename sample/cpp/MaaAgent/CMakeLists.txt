add_executable(agent_child agent_child.cpp)
target_link_libraries(agent_child MaaAgentServer MaaToolkit)

add_executable(agent_main agent_main.cpp)
target_link_libraries(agent_main MaaFramework MaaToolkit MaaAgentClient)

add_dependencies(agent_main agent_child MaaAgentClient)
add_dependencies(agent_child MaaAgentServer MaaToolkit)

if(MSVC)
    target_compile_options(agent_child PRIVATE "/WX-")
    target_compile_options(agent_main PRIVATE "/WX-")
else()
    target_compile_options(agent_child PRIVATE -Wno-error -Wno-unused-parameter -Wno-unused-variable)
    target_compile_options(agent_main PRIVATE -Wno-error -Wno-unused-parameter -Wno-unused-variable)
endif()
