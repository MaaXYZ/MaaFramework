file(
    GLOB_RECURSE
    testing_src
    *.cpp
    *.h
    *.hpp)
    
add_executable(testing ${testing_src})

target_link_libraries(testing MaaFramework)

add_dependencies(testing MaaFramework)

install(TARGETS testing RUNTIME DESTINATION bin)
