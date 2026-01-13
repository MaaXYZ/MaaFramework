file(
    GLOB_RECURSE
    dlopen_testing_src
    *.cpp
    *.h
    *.hpp)

add_executable(DlopenTesting ${dlopen_testing_src})

target_include_directories(DlopenTesting
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PUBLIC_INC} ${MAA_PRIVATE_INC})

target_link_libraries(DlopenTesting LibraryHolder)

if(LINUX)
    target_link_libraries(DlopenTesting dl)
endif()

if(WITH_ADB_CONTROLLER)
    target_compile_definitions(DlopenTesting PRIVATE WITH_ADB_CONTROLLER)
endif()

if(WITH_WIN32_CONTROLLER)
    target_compile_definitions(DlopenTesting PRIVATE WITH_WIN32_CONTROLLER)
endif()

if(WITH_PLAYCOVER_CONTROLLER)
    target_compile_definitions(DlopenTesting PRIVATE WITH_PLAYCOVER_CONTROLLER)
endif()

if(WITH_DBG_CONTROLLER)
    target_compile_definitions(DlopenTesting PRIVATE WITH_DBG_CONTROLLER)
endif()

if(WITH_CUSTOM_CONTROLLER)
    target_compile_definitions(DlopenTesting PRIVATE WITH_CUSTOM_CONTROLLER)
endif()

if(WITH_GAMEPAD_CONTROLLER)
    target_compile_definitions(DlopenTesting PRIVATE WITH_GAMEPAD_CONTROLLER)
endif()

add_dependencies(DlopenTesting LibraryHolder)

set_target_properties(DlopenTesting PROPERTIES FOLDER Testing)

install(TARGETS DlopenTesting RUNTIME DESTINATION bin)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:DlopenTesting> DESTINATION symbol OPTIONAL)
endif()
