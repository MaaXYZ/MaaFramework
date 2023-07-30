add_executable(serverSample main.cpp)

target_link_libraries(serverSample MaaToolKit)

add_dependencies(serverSample AssetsControllerConfig AssetsAgentBinary AssetsResource)

install(TARGETS serverSample RUNTIME DESTINATION bin)
