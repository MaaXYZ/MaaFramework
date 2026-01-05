# DPI 截图问题测试程序（仅 Windows）
add_executable(dpi_test dpi_test.cpp)

if(MSVC)
    target_compile_options(dpi_test PRIVATE "/WX-")

    # 禁用 MSVC 默认的 Per-Monitor DPI Aware 设置，强制 DPI Unaware 以复现问题
    target_link_options(dpi_test PRIVATE "/MANIFEST:EMBED" "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/dpi_test.exe.manifest")
endif()

target_link_libraries(dpi_test MaaFramework MaaToolkit)
