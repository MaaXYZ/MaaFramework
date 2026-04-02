file(GLOB_RECURSE maa_android_native_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_android_native_control_unit_header ${MAA_PUBLIC_INC}/MaaControlUnit/AndroidNativeControlUnitAPI.h ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)

add_library(MaaAndroidNativeControlUnit SHARED ${maa_android_native_control_unit_src} ${maa_android_native_control_unit_header})

target_include_directories(MaaAndroidNativeControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaAndroidNativeControlUnit PRIVATE MaaUtils ${OpenCV_LIBS} Boost::system ${CMAKE_DL_LIBS})

target_compile_definitions(MaaAndroidNativeControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaAndroidNativeControlUnit MaaUtils)

install(
    TARGETS MaaAndroidNativeControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_android_native_control_unit_src})
