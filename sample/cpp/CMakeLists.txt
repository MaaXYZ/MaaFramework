add_executable(sample main.cpp)
target_link_libraries(sample MaaFramework MaaToolkit)

add_executable(sample_cpp main_cpp.cpp)
target_link_libraries(sample_cpp MaaFramework MaaToolkit)

if(WITH_MAA_AGENT)
    add_subdirectory(MaaAgent)
endif()
