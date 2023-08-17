add_executable(sample main.cpp)

target_link_libraries(sample MaaFramework MaaToolKit)

add_dependencies(sample AssetsControllerConfig AssetsAgentBinary AssetsResource)

install(TARGETS sample RUNTIME DESTINATION share)
