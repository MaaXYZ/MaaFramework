add_executable(sample main.cpp)

target_link_libraries(sample MaaFramework)

add_dependencies(sample AssetsControllerConfig AssetsAgentBinary AssetsResource)
