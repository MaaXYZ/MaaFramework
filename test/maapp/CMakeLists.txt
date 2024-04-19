file(GLOB_RECURSE maapp_test *.cpp *.h *.hpp)

add_executable(MaaPPTest ${maapp_test})

target_include_directories(MaaPPTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PUBLIC_INC})
target_link_libraries(MaaPPTest PRIVATE MaaFramework MaaToolkit HeaderOnlyLibraries)

set_target_properties(MaaPPTest PROPERTIES FOLDER Testing)

install(TARGETS MaaPPTest RUNTIME DESTINATION bin)
