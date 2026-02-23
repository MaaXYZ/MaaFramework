add_executable(macos_test
    MacOSPermission.h MacOSPermission.mm
    MacOSTestGUI.h MacOSTestGUI.mm
    main.cpp
)

target_compile_options(macos_test PRIVATE -Wno-error -Wno-unused-parameter -Wno-unused-variable)

find_library(CORE_GRAPHICS CoreGraphics REQUIRED)
find_library(APPLICATION_SERVICES ApplicationServices REQUIRED)
find_library(FOUNDATION Foundation REQUIRED)
find_library(APPKIT AppKit REQUIRED)
find_library(CARBON Carbon REQUIRED)
target_link_libraries(macos_test MaaFramework MaaToolkit ${CORE_GRAPHICS} ${APPLICATION_SERVICES} ${FOUNDATION} ${APPKIT} ${CARBON})
