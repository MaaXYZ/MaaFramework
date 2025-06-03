file(
    GLOB_RECURSE
    node_test_src
    *.cpp
    *.h
    *.hpp)

add_executable(NodeTest ${node_test_src})

target_link_libraries(NodeTest MaaFramework MaaToolkit HeaderOnlyLibraries)

add_dependencies(NodeTest MaaFramework)
set_target_properties(NodeTest PROPERTIES FOLDER Testing)

install(TARGETS NodeTest RUNTIME DESTINATION bin)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:NodeTest> DESTINATION symbol OPTIONAL)
endif()
