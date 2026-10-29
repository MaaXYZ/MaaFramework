cmake_minimum_required(VERSION 3.28)

project(MaaFrameworkPackageTest LANGUAGES CXX)

find_package(MaaFramework CONFIG REQUIRED)

enable_testing()

add_executable(MaaFrameworkPackageTest main.cpp)
target_compile_features(MaaFrameworkPackageTest PRIVATE cxx_std_20)
target_link_libraries(
    MaaFrameworkPackageTest
    PRIVATE MaaFramework::MaaFramework
            MaaFramework::MaaToolkit
            MaaFramework::MaaAgentClient
            MaaFramework::MaaUtils)

add_test(NAME MaaFrameworkPackageTest COMMAND MaaFrameworkPackageTest)
