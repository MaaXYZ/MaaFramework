file(GLOB_RECURSE maapp_module_src **.cppm)

add_library(MaaPPModule)
target_sources(MaaPPModule PUBLIC FILE_SET CXX_MODULES FILES ${maapp_module_src})
target_include_directories(MaaPPModule PUBLIC include)
target_compile_definitions(MaaPPModule PUBLIC MAAPP_USE_MODULE)
target_include_directories(MaaPPModule INTERFACE include)
target_link_libraries(MaaPPModule PUBLIC MaaFramework MaaToolkit HeaderOnlyLibraries)

add_library(MaaPP INTERFACE)
target_include_directories(MaaPP INTERFACE include)
