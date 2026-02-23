add_executable(macos_test
    MacOSPermission.h MacOSPermission.mm
    MacOSTestGUI.h MacOSTestGUI.mm
    main.cpp
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pipeline.json ${CMAKE_BINARY_DIR}/resources/macos_test/pipeline/pipeline.json COPYONLY)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/resources/macos_test/image/Bee.png
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/GenerateBee.py
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/resources/macos_test/image
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/GenerateBee.py
)
add_custom_target(generate_bee_png DEPENDS ${CMAKE_BINARY_DIR}/resources/macos_test/image/Bee.png)
add_dependencies(macos_test generate_bee_png)

target_compile_options(macos_test PRIVATE -Wno-error -Wno-unused-parameter -Wno-unused-variable)

find_library(CORE_GRAPHICS CoreGraphics REQUIRED)
find_library(APPLICATION_SERVICES ApplicationServices REQUIRED)
find_library(FOUNDATION Foundation REQUIRED)
find_library(APPKIT AppKit REQUIRED)
find_library(CARBON Carbon REQUIRED)
target_link_libraries(macos_test MaaFramework MaaToolkit ${CORE_GRAPHICS} ${APPLICATION_SERVICES} ${FOUNDATION} ${APPKIT} ${CARBON})
