cmake_minimum_required(VERSION 3.28)

project(MaaPiCliPackageTest LANGUAGES CXX)

find_package(MaaPiCli 0.1.0 CONFIG REQUIRED)

enable_testing()

add_executable(MaaPiCliPackageTest main.cpp)
target_compile_features(MaaPiCliPackageTest PRIVATE cxx_std_20)
target_link_libraries(MaaPiCliPackageTest PRIVATE MaaPiCli::Core)

add_test(NAME MaaPiCliPackageTest COMMAND MaaPiCliPackageTest)
