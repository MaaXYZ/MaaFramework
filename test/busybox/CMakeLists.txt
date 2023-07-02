add_executable(MaaBusybox maabusybox.cpp)

target_compile_definitions(MaaBusybox PRIVATE MAA_DEBUG)
target_link_libraries(MaaBusybox MaaFramework HeaderOnlyLibraries)
target_include_directories(MaaBusybox PUBLIC ${MaaFramework_INCLUDE} "../../source")

add_dependencies(MaaBusybox AssetsControllerConfig AssetsAgentBinary)
