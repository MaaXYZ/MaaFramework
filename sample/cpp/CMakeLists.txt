add_executable(sample main.cpp)

target_link_libraries(sample MaaFramework MaaToolKit)

add_dependencies(sample AssetsAgentBinary AssetsResource)

install(TARGETS sample RUNTIME DESTINATION share)
