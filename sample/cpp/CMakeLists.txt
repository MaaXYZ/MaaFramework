add_executable(sample main.cpp)
target_link_libraries(sample MaaFramework MaaToolkit)

if(WITH_MAA_AGENT)
    add_subdirectory(MaaAgent)
endif()