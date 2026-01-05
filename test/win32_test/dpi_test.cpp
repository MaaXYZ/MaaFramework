/**
 * DPI 截图问题复现测试程序
 *
 * 用法：
 * 1. 将 Windows 屏幕缩放设置为 175% 或 150%
 * 2. 编译并运行此程序
 * 3. 程序会截取指定窗口并保存截图
 * 4. 对比修复前后的截图是否完整
 *
 * 此程序故意设置为 DPI Unaware 模式来复现用户反馈的问题
 */

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "MaaFramework/MaaAPI.h"
#include "MaaToolkit/MaaToolkitAPI.h"

// 保存截图到文件
void save_screenshot(MaaController* controller, const std::string& filename)
{
    auto image = MaaImageBufferCreate();

    // 先截一张图
    auto id = MaaControllerPostScreencap(controller);
    MaaControllerWait(controller, id);
    MaaControllerCachedImage(controller, image);

    int32_t width = MaaImageBufferWidth(image);
    int32_t height = MaaImageBufferHeight(image);

    std::cout << "Screenshot size: " << width << "x" << height << std::endl;

    // 保存 PNG
    uint8_t* png_data = MaaImageBufferGetEncoded(image);
    size_t png_size = MaaImageBufferGetEncodedSize(image);

    if (png_data && png_size > 0) {
        std::ofstream file(filename, std::ios::binary);
        file.write(reinterpret_cast<char*>(png_data), png_size);
        file.close();
        std::cout << "Saved screenshot to: " << filename << std::endl;
    }

    MaaImageBufferDestroy(image);
}

// 获取 DPI 感知模式的字符串描述
const char* get_dpi_awareness_name(DPI_AWARENESS_CONTEXT ctx)
{
    if (AreDpiAwarenessContextsEqual(ctx, DPI_AWARENESS_CONTEXT_UNAWARE)) {
        return "UNAWARE";
    }
    if (AreDpiAwarenessContextsEqual(ctx, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE)) {
        return "SYSTEM_AWARE";
    }
    if (AreDpiAwarenessContextsEqual(ctx, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE)) {
        return "PER_MONITOR_AWARE";
    }
    if (AreDpiAwarenessContextsEqual(ctx, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        return "PER_MONITOR_AWARE_V2";
    }
    if (AreDpiAwarenessContextsEqual(ctx, DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED)) {
        return "UNAWARE_GDISCALED";
    }
    return "UNKNOWN";
}

int main()
{
#ifdef _WIN32
    std::cout << "=== DPI Screenshot Test ===" << std::endl;
    std::cout << "PID: " << GetCurrentProcessId() << std::endl;

    // 获取设置前的 DPI 感知模式
    auto before_ctx = GetThreadDpiAwarenessContext();
    std::cout << "DPI Awareness BEFORE: " << get_dpi_awareness_name(before_ctx) << std::endl;

    // ============================================================
    // 【关键】强制设置为 DPI Unaware 模式来复现问题
    // 修复后，即使设置为 Unaware，截图也应该是完整的
    // ============================================================
    auto prev = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);
    if (!prev) {
        DWORD err = GetLastError();
        std::cout << "!!! SetProcessDpiAwarenessContext FAILED !!!" << std::endl;
        std::cout << "Error code: " << err << std::endl;
        if (err == ERROR_ACCESS_DENIED) {
            std::cout << "Reason: DPI awareness already set (by manifest or earlier call)" << std::endl;
        }
    }
    else {
        std::cout << "SetProcessDpiAwarenessContext: SUCCESS" << std::endl;
    }

    // 获取设置后的 DPI 感知模式
    auto after_ctx = GetThreadDpiAwarenessContext();
    std::cout << "DPI Awareness AFTER: " << get_dpi_awareness_name(after_ctx) << std::endl;
    std::cout << std::endl;
#endif

    MaaToolkitConfigInitOption("./", "{}");

    // 查找窗口
    auto list_handle = MaaToolkitDesktopWindowListCreate();
    MaaToolkitDesktopWindowFindAll(list_handle);

    size_t size = MaaToolkitDesktopWindowListSize(list_handle);
    if (size == 0) {
        std::cout << "No window found" << std::endl;
        MaaToolkitDesktopWindowListDestroy(list_handle);
        return -1;
    }

    std::cout << "Found " << size << " windows:" << std::endl;
    for (size_t i = 0; i < size && i < 20; ++i) {
        auto window = MaaToolkitDesktopWindowListAt(list_handle, i);
        std::string name = MaaToolkitDesktopWindowGetWindowName(window);
        if (!name.empty()) {
            std::cout << "  [" << i << "] " << name << std::endl;
        }
    }

    std::cout << std::endl << "Enter window index to test (or window name keyword): ";
    std::string input;
    std::getline(std::cin, input);

    void* hwnd = nullptr;
    std::string window_name;

    // 尝试解析为数字索引
    try {
        size_t index = std::stoul(input);
        if (index < size) {
            auto window = MaaToolkitDesktopWindowListAt(list_handle, index);
            hwnd = MaaToolkitDesktopWindowGetHandle(window);
            window_name = MaaToolkitDesktopWindowGetWindowName(window);
        }
    }
    catch (...) {
        // 按名称搜索
        for (size_t i = 0; i < size; ++i) {
            auto window = MaaToolkitDesktopWindowListAt(list_handle, i);
            std::string name = MaaToolkitDesktopWindowGetWindowName(window);
            if (name.find(input) != std::string::npos) {
                hwnd = MaaToolkitDesktopWindowGetHandle(window);
                window_name = name;
                break;
            }
        }
    }

    if (!hwnd) {
        std::cout << "Window not found" << std::endl;
        MaaToolkitDesktopWindowListDestroy(list_handle);
        return -1;
    }

    std::cout << "Selected window: " << window_name << std::endl;
    std::cout << std::endl;

    // 获取窗口信息用于验证
    RECT client_rect = { 0 };
    GetClientRect((HWND)hwnd, &client_rect);
    int logical_width = client_rect.right - client_rect.left;
    int logical_height = client_rect.bottom - client_rect.top;
    std::cout << "GetClientRect (logical/virtualized): " << logical_width << "x" << logical_height << std::endl;

    // 获取窗口 DPI 来计算物理大小
    UINT dpi = GetDpiForWindow((HWND)hwnd);
    double scale = dpi / 96.0;
    std::cout << "Window DPI: " << dpi << " (scale: " << scale << "x)" << std::endl;
    std::cout << "Expected physical size: " << (int)(logical_width * scale) << "x" << (int)(logical_height * scale) << std::endl;
    std::cout << std::endl;
    std::cout << "If screenshot size matches LOGICAL size but not PHYSICAL size," << std::endl;
    std::cout << "the DPI issue is reproduced!" << std::endl;

    // 测试不同的截图方法
    struct ScreencapMethod
    {
        MaaWin32ScreencapMethod method;
        const char* name;
    };

    ScreencapMethod methods[] = {
        { MaaWin32ScreencapMethod_GDI, "GDI" },
        { MaaWin32ScreencapMethod_DXGI_DesktopDup, "DXGI_DesktopDup" },
        { MaaWin32ScreencapMethod_DXGI_DesktopDup_Window, "DXGI_DesktopDup_Window" },
        { MaaWin32ScreencapMethod_FramePool, "FramePool" },
        { MaaWin32ScreencapMethod_PrintWindow, "PrintWindow" },
        { MaaWin32ScreencapMethod_ScreenDC, "ScreenDC" },
    };

    for (const auto& m : methods) {
        std::cout << std::endl << "Testing: " << m.name << std::endl;

        auto controller = MaaWin32ControllerCreate(hwnd, m.method, MaaWin32InputMethod_SendMessage, MaaWin32InputMethod_SendMessage);

        auto id = MaaControllerPostConnection(controller);
        MaaControllerWait(controller, id);

        id = MaaControllerPostScreencap(controller);
        MaaControllerWait(controller, id);

        std::string filename = std::string("screenshot_") + m.name + ".png";
        save_screenshot(controller, filename);

        MaaControllerDestroy(controller);
    }

    MaaToolkitDesktopWindowListDestroy(list_handle);

    std::cout << std::endl << "=== Test Complete ===" << std::endl;
    std::cout << "Compare the saved screenshots to check if they are complete." << std::endl;
    std::cout << "If FramePool/PrintWindow screenshots show only top-left corner," << std::endl;
    std::cout << "the DPI issue is reproduced." << std::endl;

    return 0;
}

