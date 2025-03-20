cmake_minimum_required(VERSION 3.28)
project(test)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../install/share/cmake")

find_package(MaaFramework CONFIG REQUIRED)

add_executable(test main.cpp)
target_link_libraries(test
    MaaFramework::MaaFramework
    MaaFramework::MaaToolkit
    MaaFramework::MaaAgentClient
    MaaFramework::MaaFrameworkModule # for cpp modules
)
