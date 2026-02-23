#include <chrono>
#include <iostream>
#include <thread>

#include "MaaFramework/MaaAPI.h"
#include "MaaToolkit/MaaToolkitAPI.h"

#include "./gui.h"
#include "./macos_permission.h"

// 检查并请求必要权限
void checkAndRequestPermissions()
{
    bool has_screen_recording_permission = check_screen_recording_permission();
    bool has_accessibility = check_accessibility_permission();

    std::cout << "屏幕录制权限: " << (has_screen_recording_permission ? "已授权" : "未授权") << std::endl;
    std::cout << "辅助功能权限: " << (has_accessibility ? "已授权" : "未授权") << std::endl;

    // 屏幕录制权限
    while (!has_screen_recording_permission) {
        std::cout << "尝试请求屏幕录制权限..." << std::endl;
        // open_screen_recording_privacy_settings();
        request_screen_recording_permission();
        std::cout << "已发送请求，请在 系统设置 -> 隐私与安全性 -> 录屏与系统录音 中授予本应用权限。" << std::endl;

        std::cout << "是否重新检查？(Y/n): ";
        std::string choice;
        std::getline(std::cin, choice);
        if (choice == "n" || choice == "N") {
            std::cout << "请在授予权限后重启应用。" << std::endl;
            exit(0);
        }
        has_screen_recording_permission = check_screen_recording_permission();
    }

    // 辅助功能权限
    while (!has_accessibility) {
        std::cout << "尝试请求辅助功能权限..." << std::endl;
        // open_accessibility_privacy_settings();
        request_accessibility_permission();
        std::cout << "已发送请求，请在 系统设置 -> 隐私与安全性 -> 辅助功能 中授予本应用权限。" << std::endl;

        std::cout << "是否重新检查？(Y/n): ";
        std::string choice2;
        std::getline(std::cin, choice2);
        if (choice2 == "n" || choice2 == "N") {
            std::cout << "请在授予权限后重启应用。" << std::endl;
            exit(0);
        }
        has_accessibility = check_accessibility_permission();
    }
}

// 运行 MaaFW 窗口测试
void runMaaTest(const std::string& window_title)
{
    // 初始化 MaaToolkit
    MaaToolkitConfigInitOption("./", "{}");

    // 等待一下，让窗口完全显示
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    uint32_t windowID = 0;

    // 使用 MaaFW 查找窗口
    auto winlist = MaaToolkitDesktopWindowListCreate();
    MaaToolkitDesktopWindowFindAll(winlist);

    size_t size = MaaToolkitDesktopWindowListSize(winlist);
    std::cout << "Found " << size << " windows." << std::endl;

    for (size_t i = 0; i < size; ++i) {
        auto w = MaaToolkitDesktopWindowListAt(winlist, i);
        std::string name = MaaToolkitDesktopWindowGetWindowName(w);
        windowID = reinterpret_cast<uintptr_t>(MaaToolkitDesktopWindowGetHandle(w));

        if (name.find(window_title) != std::string::npos) {
            std::cout << "Found Test Window: " << name << " (WindowID: " << windowID << ")" << std::endl;
        }
    }

    MaaToolkitDesktopWindowListDestroy(winlist);

    if (windowID == 0) {
        std::cout << "未找到测试窗口，无法创建 MaaController。" << std::endl;
        exit(-1);
    }

    // 创建控制器
    auto controller = MaaMacOSControllerCreate(windowID, MaaMacOSScreencapMethod_ScreenCaptureKit, MaaMacOSInputMethod_GlobalEvent);
    if (!controller) {
        std::cout << "创建 MaaController 失败。" << std::endl;
        exit(-1);
    }

    std::cout << "MaaController 创建成功，开始测试输入..." << std::endl;
    
}

void gui(const std::string& window_title)
{
    createAndShowTestWindow(window_title);
    runAppLoop();
}

int main()
{
    checkAndRequestPermissions();
    std::string random_title = "MaaTestWindow_" + std::to_string(rand() % 10000);
    std::thread maatThread(runMaaTest, random_title);
    gui(random_title);

    maatThread.join();
    return 0;
}
