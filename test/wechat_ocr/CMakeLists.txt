string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" wechat_test_arch)
if(CMAKE_GENERATOR_PLATFORM)
    string(TOLOWER "${CMAKE_GENERATOR_PLATFORM}" wechat_test_arch)
endif()

if((WIN32 OR (LINUX AND NOT ANDROID))
   AND CMAKE_SIZEOF_VOID_P EQUAL 8
   AND wechat_test_arch MATCHES "^(x86_64|amd64|x64)(,.*)?$")
    foreach(wechat_test_target IN ITEMS WeChatOCRTestSDK WeChatOCRLegacyTestSDK)
        add_library(${wechat_test_target} SHARED fake_sdk.cpp)
        target_compile_features(${wechat_test_target} PRIVATE cxx_std_20)
        set_target_properties(${wechat_test_target} PROPERTIES FOLDER Testing CXX_VISIBILITY_PRESET hidden)
    endforeach()
    target_compile_definitions(WeChatOCRLegacyTestSDK PRIVATE MAA_WECHAT_TEST_LEGACY)
    install(
        TARGETS WeChatOCRTestSDK WeChatOCRLegacyTestSDK
        RUNTIME DESTINATION test/WeChatOCR COMPONENT WeChatOCRTests
        LIBRARY DESTINATION test/WeChatOCR COMPONENT WeChatOCRTests)
endif()
