set(maa_ios_control_unit_dir "${CMAKE_SOURCE_DIR}/source/MaaIOSControlUnit")

if(MAA_IOS_PROTOCOL_TESTS)
    set(maa_ios_protocol_test_names plist json hid rtp ddi_selection ddi_mounter discovery device_selection tls_embedding credentials decoder_capability)
    foreach(name IN LISTS maa_ios_protocol_test_names)
        add_executable(maa_ios_${name}_test protocol/${name}_test.cpp)
        target_link_libraries(maa_ios_${name}_test PRIVATE MaaIOSProtocol)
        add_test(NAME maa_ios_${name} COMMAND maa_ios_${name}_test)
        set_tests_properties(maa_ios_${name} PROPERTIES TIMEOUT 30)
    endforeach()
    add_executable(maa_ios_frame_delivery_test protocol/frame_delivery_test.cpp "${maa_ios_control_unit_dir}/Protocol/media/FramePump.cpp")
    target_link_libraries(maa_ios_frame_delivery_test PRIVATE MaaIOSProtocol)
    add_test(NAME maa_ios_frame_delivery COMMAND maa_ios_frame_delivery_test)
    set_tests_properties(maa_ios_frame_delivery PROPERTIES TIMEOUT 15)
endif()

if(BUILD_IOS_CONTROLLER_TESTING)
    add_executable(MaaIOSInputMappingTest ios_input_mapping_test.cpp)
    target_include_directories(MaaIOSInputMappingTest PRIVATE "${maa_ios_control_unit_dir}")
    add_test(NAME MaaIOSInputMappingTest COMMAND MaaIOSInputMappingTest)

    add_executable(MaaIOSManagementAPITest ios_management_api_test.cpp)
    target_include_directories(MaaIOSManagementAPITest PRIVATE ${MAA_PUBLIC_INC} "${MAAUTILS_DIR}/include")
    target_link_libraries(MaaIOSManagementAPITest PRIVATE MaaToolkit MaaUtils ${OpenCV_LIBS})
    add_test(NAME MaaIOSManagementAPITest COMMAND MaaIOSManagementAPITest)

    add_executable(MaaIOSToolkitTest
        toolkit_device_test.cpp
        ${CMAKE_SOURCE_DIR}/source/MaaToolkit/API/MaaToolkitIOSDevice.cpp)
    target_include_directories(MaaIOSToolkitTest PRIVATE ${MAA_PUBLIC_INC} ${MAA_PRIVATE_INC}
        "${MAAUTILS_DIR}/include")
    target_compile_definitions(MaaIOSToolkitTest PRIVATE MAA_WITH_IOS_DEVICE=1 MAA_TOOLKIT_EXPORTS)
    target_link_libraries(MaaIOSToolkitTest PRIVATE MaaFramework MaaUtils Boost::system ${OpenCV_LIBS})
    add_test(NAME MaaIOSToolkitTest COMMAND MaaIOSToolkitTest)
endif()
