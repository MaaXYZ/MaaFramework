add_library(MaaPluginDemo SHARED main.cpp)

target_link_libraries(MaaPluginDemo PRIVATE MaaFramework)

set_target_properties(MaaPluginDemo PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug/plugins
                                               LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release/plugins)
