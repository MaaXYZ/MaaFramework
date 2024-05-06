add_executable(sample main.cpp)
target_link_libraries(sample MaaFramework MaaToolkit)

set(MaaPP_INC_REL ${CMAKE_CURRENT_LIST_DIR}/../../source/binding/cpp/include)
cmake_path(ABSOLUTE_PATH MaaPP_INC_REL NORMALIZE OUTPUT_VARIABLE MaaPP_INC)

add_executable(sample_maapp_coro main_maapp_coro.cpp)
target_include_directories(sample_maapp_coro PRIVATE ${MaaPP_INC})
target_link_libraries(sample_maapp_coro MaaFramework MaaToolkit HeaderOnlyLibraries)
