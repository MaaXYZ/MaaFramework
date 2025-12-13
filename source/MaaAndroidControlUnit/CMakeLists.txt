if(NOT ANDROID)
    message(STATUS "MaaAndroidControlUnit skipped: not on Android")
    return()
endif()

file(GLOB_RECURSE maa_android_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_android_control_unit_header ${MAA_PRIVATE_INC}/ControlUnit/AndroidControlUnitAPI.h ${MAA_PRIVATE_INC}/ControlUnit/ControlUnitAPI.h)

add_library(MaaAndroidControlUnit SHARED ${maa_android_control_unit_src} ${maa_android_control_unit_header})

target_include_directories(MaaAndroidControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_compile_definitions(MaaAndroidControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

target_link_libraries(MaaAndroidControlUnit
    PRIVATE MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS} log jnigraphics android)

add_dependencies(MaaAndroidControlUnit MaaUtils)

install(
    TARGETS MaaAndroidControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    # ARCHIVE DESTINATION lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_android_control_unit_src})
