file(GLOB_RECURSE maa_android_native_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_android_native_control_unit_header ${MAA_PRIVATE_INC}/ControlUnit/AndroidNativeControlUnitAPI.h ${MAA_PRIVATE_INC}/ControlUnit/ControlUnitAPI.h)

add_library(MaaAndroidNativeControlUnit SHARED ${maa_android_native_control_unit_src} ${maa_android_native_control_unit_header})

target_include_directories(MaaAndroidNativeControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaAndroidNativeControlUnit PRIVATE MaaUtils ${OpenCV_LIBS} ZLIB::ZLIB Boost::system)
target_link_libraries(MaaAndroidNativeControlUnit PRIVATE android log)

target_compile_definitions(MaaAndroidNativeControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaAndroidNativeControlUnit MaaUtils)

install(
    TARGETS MaaAndroidNativeControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_android_native_control_unit_src})
