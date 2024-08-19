file(GLOB_RECURSE codec_test_src *.cpp *.h *.hpp)

add_executable(CodecTest ${codec_test_src})

target_include_directories(CodecTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PUBLIC_INC} ${MAA_PRIVATE_INC})

target_link_libraries(CodecTest PRIVATE MaaUtils)

set_target_properties(CodecTest PROPERTIES FOLDER Testing)

install(TARGETS CodecTest RUNTIME DESTINATION bin)
