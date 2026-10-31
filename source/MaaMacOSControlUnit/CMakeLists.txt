file(GLOB_RECURSE maa_macos_control_unit_src *.h *.hpp *.cpp *.mm)
file(GLOB_RECURSE maa_macos_control_unit_header ${MAA_PUBLIC_INC}/MaaControlUnit/MacOSControlUnitAPI.h ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)

set(maa_macos_app_lifecycle_src
    ${CMAKE_CURRENT_SOURCE_DIR}/../ControlUnitCommon/MacOS/MacOSAppLifecycle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../ControlUnitCommon/MacOS/MacOSAppLifecycle.mm)

add_library(MaaMacOSControlUnit SHARED ${maa_macos_control_unit_src} ${maa_macos_control_unit_header} ${maa_macos_app_lifecycle_src})

target_include_directories(MaaMacOSControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../ControlUnitCommon ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaMacOSControlUnit PRIVATE MaaUtils ${OpenCV_LIBS})

# macOS frameworks
find_library(APPLICATION_SERVICES ApplicationServices REQUIRED)
find_library(CORE_FOUNDATION CoreFoundation REQUIRED)
find_library(FOUNDATION Foundation REQUIRED)
find_library(SCREEN_CAPTURE_KIT ScreenCaptureKit REQUIRED)
find_library(CORE_MEDIA CoreMedia REQUIRED)
find_library(CORE_VIDEO CoreVideo REQUIRED)
find_library(APP_KIT AppKit REQUIRED)
find_library(CARBON Carbon REQUIRED)

target_link_libraries(MaaMacOSControlUnit PRIVATE
    ${APPLICATION_SERVICES}
    ${CORE_FOUNDATION}
    ${FOUNDATION}
    ${SCREEN_CAPTURE_KIT}
    ${CORE_MEDIA}
    ${CORE_VIDEO}
    ${APP_KIT}
    ${CARBON}
)

target_compile_definitions(MaaMacOSControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaMacOSControlUnit MaaUtils)

install(
    TARGETS MaaMacOSControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin

    # ARCHIVE DESTINATION lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_macos_control_unit_src})

source_group(PREFIX "ControlUnitCommon" TREE ${CMAKE_CURRENT_SOURCE_DIR}/../ControlUnitCommon FILES ${maa_macos_app_lifecycle_src})
