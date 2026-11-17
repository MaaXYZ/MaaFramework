cmake_minimum_required(VERSION 3.28)

project(MaaFrameworkPackageTest LANGUAGES CXX)

set(control_unit_components MaaCustomControlUnit)
if(WIN32)
    list(APPEND control_unit_components MaaWin32ControlUnit)
endif()
find_package(MaaFramework CONFIG REQUIRED COMPONENTS ${control_unit_components})

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

foreach(backend IN LISTS control_unit_components)
    add_executable(${backend}PackageTest ${backend}.cpp)
    target_link_libraries(${backend}PackageTest PRIVATE MaaFramework::${backend})
    add_test(NAME ${backend}PackageTest COMMAND ${backend}PackageTest)
endforeach()
