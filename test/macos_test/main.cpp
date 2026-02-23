#include <iostream>

#include "./macos_permission.h"

int main()
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
