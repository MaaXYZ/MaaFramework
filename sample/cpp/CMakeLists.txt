add_executable(sample main.cpp)
target_link_libraries(sample MaaFramework MaaToolkit)

add_executable(sample_maapp_coro main_maapp_coro.cpp)
target_include_library(sample_maapp_coro PRIVATE "../../source/binding")
target_link_libraries(sample_maapp_coro MaaFramework MaaToolkit HeaderOnlyLibraries)
