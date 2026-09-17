add_executable(AgentFaultServer fault_server.cpp)

target_include_directories(AgentFaultServer PRIVATE ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})
target_link_libraries(AgentFaultServer PRIVATE cppzmq-static)

set_target_properties(AgentFaultServer PROPERTIES FOLDER Testing)

install(TARGETS AgentFaultServer RUNTIME DESTINATION bin)
