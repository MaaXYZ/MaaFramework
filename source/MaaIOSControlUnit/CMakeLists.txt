add_library(MaaIOSControlUnit SHARED
    API/IOSControlUnitAPI.cpp
    Manager/IOSControlUnitMgr.cpp
    Manager/IOSControlUnitMgr.h
    Session/IOSSession.cpp
    Session/IOSSession.h
    Session/InputMapping.h
    Session/HeldInputs.h)
target_include_directories(MaaIOSControlUnit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})
target_link_libraries(MaaIOSControlUnit PRIVATE MaaUtils MaaIOSProtocol ${OpenCV_LIBS})
target_compile_definitions(MaaIOSControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)
install(TARGETS MaaIOSControlUnit RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

option(BUILD_IOS_CONTROLLER_TESTING "Build offline iOS controller tests" OFF)
if(BUILD_IOS_CONTROLLER_TESTING)
    enable_testing()
    add_executable(MaaIOSInputMappingTest ${CMAKE_SOURCE_DIR}/test/ios_test/ios_input_mapping_test.cpp)
    target_include_directories(MaaIOSInputMappingTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME MaaIOSInputMappingTest COMMAND MaaIOSInputMappingTest)
endif()
