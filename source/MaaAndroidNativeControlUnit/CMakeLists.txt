file(GLOB_RECURSE maa_android_native_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_android_native_control_unit_header ${MAA_PUBLIC_INC}/MaaControlUnit/AndroidNativeControlUnitAPI.h ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)

add_control_unit(MaaAndroidNativeControlUnit ${maa_android_native_control_unit_src} ${maa_android_native_control_unit_header})

target_link_libraries(MaaAndroidNativeControlUnit PRIVATE Boost::system ${CMAKE_DL_LIBS})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_android_native_control_unit_src})
