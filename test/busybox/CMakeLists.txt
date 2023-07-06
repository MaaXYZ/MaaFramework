add_executable(MaaBusybox maabusybox.cpp)

target_link_libraries(MaaBusybox MaaFramework HeaderOnlyLibraries)
target_include_directories(MaaBusybox PUBLIC ${MaaFramework_INCLUDE} "../../source" "../../source/modules/include")

add_dependencies(MaaBusybox AssetsControllerConfig AssetsAgentBinary AssetsResource)
