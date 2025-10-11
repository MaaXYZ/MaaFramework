file(GLOB_RECURSE dummy_test_src *.cpp *.h *.hpp *.mm)

find_library(CocoaLib Cocoa)

add_executable(DummyTest ${dummy_test_src})

target_include_directories(DummyTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PUBLIC_INC} ${MAA_PRIVATE_INC})

target_link_libraries(DummyTest MaaFramework MaaToolkit)
