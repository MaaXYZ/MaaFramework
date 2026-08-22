add_executable(MaaPiCliTesting main.cpp)

target_link_libraries(MaaPiCliTesting PRIVATE MaaPiCliCore)
add_dependencies(MaaPiCliTesting MaaPiCliCore)
set_target_properties(MaaPiCliTesting PROPERTIES FOLDER Testing)

install(TARGETS MaaPiCliTesting RUNTIME DESTINATION bin)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaPiCliTesting> DESTINATION symbol OPTIONAL)
endif()
