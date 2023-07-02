add_executable(sample main.cpp)

set_target_properties(sample PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(sample MaaFramework)
# target_include_directories(sample PUBLIC ${MaaFramework_INCLUDE})
