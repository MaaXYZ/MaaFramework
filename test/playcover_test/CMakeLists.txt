file(GLOB_RECURSE playcover_test_src *.cpp *.h *.hpp)

add_executable(PlayCoverTest ${playcover_test_src})

target_link_libraries(PlayCoverTest MaaFramework)

add_dependencies(PlayCoverTest MaaFramework MaaPlayCoverControlUnit)

add_custom_command(
    TARGET PlayCoverTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/resource
            $<TARGET_FILE_DIR:PlayCoverTest>/resource)

set_target_properties(PlayCoverTest PROPERTIES FOLDER Testing)

install(TARGETS PlayCoverTest RUNTIME DESTINATION bin)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource
    DESTINATION bin
    OPTIONAL
)
