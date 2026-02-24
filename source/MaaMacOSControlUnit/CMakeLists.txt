file(GLOB_RECURSE maa_macos_control_unit_src *.h *.hpp *.cpp *.mm)
file(GLOB_RECURSE maa_macos_control_unit_header ${MAA_PRIVATE_INC}/ControlUnit/MacOSControlUnitAPI.h ${MAA_PRIVATE_INC}/ControlUnit/ControlUnitAPI.h)

add_library(MaaMacOSControlUnit SHARED ${maa_macos_control_unit_src} ${maa_macos_control_unit_header})

target_include_directories(MaaMacOSControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

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
