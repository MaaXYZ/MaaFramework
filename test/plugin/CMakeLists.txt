find_package(Threads REQUIRED)

add_library(PluginTestFixture SHARED fixture.cpp)
target_compile_definitions(PluginTestFixture PRIVATE MAA_PLUGIN_EXPORTS)
target_include_directories(PluginTestFixture PUBLIC ${CMAKE_SOURCE_DIR}/3rdparty/include)
target_link_libraries(PluginTestFixture PRIVATE MaaFramework Threads::Threads)

add_executable(PluginLifecycleTesting main.cpp)
target_include_directories(PluginLifecycleTesting PRIVATE ${MAA_PUBLIC_INC})
target_link_libraries(PluginLifecycleTesting PRIVATE MaaFramework PluginTestFixture)
target_compile_definitions(PluginLifecycleTesting PRIVATE
    PLUGIN_TEST_LIBRARY="${CMAKE_SHARED_LIBRARY_PREFIX}PluginTestFixture${CMAKE_SHARED_LIBRARY_SUFFIX}")
set_target_properties(PluginTestFixture PluginLifecycleTesting PROPERTIES FOLDER Testing)
install(TARGETS PluginTestFixture PluginLifecycleTesting RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
add_test(NAME PluginLifecycle COMMAND PluginLifecycleTesting $<TARGET_FILE:PluginTestFixture>)
