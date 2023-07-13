add_executable(maahelper main.cpp)

target_link_libraries(maahelper MaaFramework)

add_dependencies(maahelper AssetsControllerConfig AssetsAgentBinary AssetsResource)
