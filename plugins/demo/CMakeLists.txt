add_library(MaaPluginDemo SHARED main.cpp)

target_link_libraries(MaaPluginDemo PRIVATE MaaFramework HeaderOnlyLibraries)

set_target_properties(
    MaaPluginDemo
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug/plugins
               RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release/plugins
               LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug/plugins
               LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release/plugins)

install(
    TARGETS MaaPluginDemo
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins)
