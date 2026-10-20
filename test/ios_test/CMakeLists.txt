set(maa_ios_control_unit_dir "${CMAKE_SOURCE_DIR}/source/MaaIOSControlUnit")

set(maa_ios_protocol_test_names Plist Json HID RTP DDISelection DDIMounter Discovery DeviceSelection TLSEmbedding Credentials)
set(maa_ios_protocol_test_sources plist json hid rtp ddi_selection ddi_mounter discovery device_selection tls_embedding credentials)
foreach(name source IN ZIP_LISTS maa_ios_protocol_test_names maa_ios_protocol_test_sources)
    add_executable(MaaIOS${name}Test protocol/${source}_test.cpp)
    target_link_libraries(MaaIOS${name}Test PRIVATE MaaIOSProtocol)
    set_target_properties(MaaIOS${name}Test PROPERTIES FOLDER Testing)
    add_test(NAME MaaIOS${name}Test COMMAND MaaIOS${name}Test)
    set_tests_properties(MaaIOS${name}Test PROPERTIES TIMEOUT 30)
endforeach()

add_executable(MaaIOSDecoderCapabilityTest protocol/decoder_capability_test.cpp)
target_link_libraries(MaaIOSDecoderCapabilityTest PRIVATE MaaIOSMedia)
add_test(NAME MaaIOSDecoderCapabilityTest COMMAND MaaIOSDecoderCapabilityTest)
set_tests_properties(MaaIOSDecoderCapabilityTest PROPERTIES TIMEOUT 30)

add_executable(MaaIOSFrameDeliveryTest protocol/frame_delivery_test.cpp)
target_link_libraries(MaaIOSFrameDeliveryTest PRIVATE MaaIOSMedia)
add_test(NAME MaaIOSFrameDeliveryTest COMMAND MaaIOSFrameDeliveryTest)
set_tests_properties(MaaIOSFrameDeliveryTest PROPERTIES TIMEOUT 15)

add_executable(MaaIOSInputMappingTest ios_input_mapping_test.cpp)
target_include_directories(MaaIOSInputMappingTest PRIVATE "${maa_ios_control_unit_dir}")
add_test(NAME MaaIOSInputMappingTest COMMAND MaaIOSInputMappingTest)

add_executable(MaaIOSManagementAPITest ios_management_api_test.cpp)
target_include_directories(MaaIOSManagementAPITest PRIVATE ${MAA_PUBLIC_INC} "${MAAUTILS_DIR}/include")
target_link_libraries(MaaIOSManagementAPITest PRIVATE MaaToolkit MaaUtils ${OpenCV_LIBS} ${CMAKE_DL_LIBS})
add_test(NAME MaaIOSManagementAPITest COMMAND MaaIOSManagementAPITest)

add_executable(MaaIOSToolkitTest
    toolkit_device_test.cpp
    ${CMAKE_SOURCE_DIR}/source/MaaToolkit/API/MaaToolkitIOSDevice.cpp)
target_include_directories(MaaIOSToolkitTest PRIVATE ${MAA_PUBLIC_INC} ${MAA_PRIVATE_INC}
    "${MAAUTILS_DIR}/include")
target_compile_definitions(MaaIOSToolkitTest PRIVATE MAA_TOOLKIT_EXPORTS)
target_link_libraries(MaaIOSToolkitTest PRIVATE MaaFramework MaaUtils Boost::system ${OpenCV_LIBS})
add_test(NAME MaaIOSToolkitTest COMMAND MaaIOSToolkitTest)

set_target_properties(
    MaaIOSDecoderCapabilityTest
    MaaIOSFrameDeliveryTest
    MaaIOSInputMappingTest
    MaaIOSManagementAPITest
    MaaIOSToolkitTest
    PROPERTIES FOLDER Testing)
