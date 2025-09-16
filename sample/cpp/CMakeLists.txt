add_executable(sample main.cpp)

if(MSVC)
    target_compile_options(sample PRIVATE "/WX-")
else()
    target_compile_options(sample PRIVATE -Wno-error -Wno-unused-parameter -Wno-unused-variable)
endif()

target_link_libraries(sample MaaFramework MaaToolkit)

if(WITH_MAA_AGENT)
    add_subdirectory(MaaAgent)
endif()
