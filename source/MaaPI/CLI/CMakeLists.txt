add_executable(MaaPICli main.cpp interactor.cpp)
target_link_libraries(MaaPICli MaaPI MaaUtils HeaderOnlyLibraries)

target_include_directories(MaaPICli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})
