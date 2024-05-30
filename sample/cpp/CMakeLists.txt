add_executable(sample main.cpp)
target_link_libraries(sample MaaFramework MaaToolkit)

set(MaaPP_INC_REL ${CMAKE_CURRENT_LIST_DIR}/../../source/binding/cpp/include)
cmake_path(ABSOLUTE_PATH MaaPP_INC_REL NORMALIZE OUTPUT_VARIABLE MaaPP_INC)

add_executable(sample_cpp20 main_c++20.cpp)
target_include_directories(sample_cpp20 PRIVATE ${MaaPP_INC})
target_link_libraries(sample_cpp20 MaaFramework MaaToolkit HeaderOnlyLibraries)
