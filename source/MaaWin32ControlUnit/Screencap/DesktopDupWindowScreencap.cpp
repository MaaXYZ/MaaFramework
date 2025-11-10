#include "DesktopDupWindowScreencap.h"

#include <algorithm>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

std::optional<cv::Mat> DesktopDupWindowScreencap::screencap()
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return std::nullopt;
    }

    // 调用基类方法获取全屏截图（BGR格式）
    auto opt_img = DesktopDupScreencap::screencap();
    if (!opt_img) {
        return std::nullopt;
    }
    const cv::Mat& img = *opt_img;

    // 获取窗口客户区在屏幕上的位置
    RECT client_rect_screen = get_window_client_rect_screen();
    if (client_rect_screen.left < 0 || client_rect_screen.top < 0 || client_rect_screen.right <= client_rect_screen.left
        || client_rect_screen.bottom <= client_rect_screen.top) {
        LogError << "Invalid client rect" << VAR(client_rect_screen.left) << VAR(client_rect_screen.top) << VAR(client_rect_screen.right)
                 << VAR(client_rect_screen.bottom);
        return std::nullopt;
    }

    int client_width = client_rect_screen.right - client_rect_screen.left;
    int client_height = client_rect_screen.bottom - client_rect_screen.top;

    // 检查裁剪区域是否在图像范围内
    if (client_rect_screen.left >= img.cols || client_rect_screen.top >= img.rows || client_rect_screen.right <= 0
        || client_rect_screen.bottom <= 0) {
        LogError << "Client rect out of bounds" << VAR(client_rect_screen.left) << VAR(client_rect_screen.top) << VAR(img.cols)
                 << VAR(img.rows);
        return std::nullopt;
    }

    // 计算裁剪区域（确保在图像范围内）
    int crop_x = std::max(0, static_cast<int>(client_rect_screen.left));
    int crop_y = std::max(0, static_cast<int>(client_rect_screen.top));
    int crop_width = std::min(client_width, img.cols - crop_x);
    int crop_height = std::min(client_height, img.rows - crop_y);

    if (crop_width <= 0 || crop_height <= 0) {
        LogError << "Invalid crop region" << VAR(crop_x) << VAR(crop_y) << VAR(crop_width) << VAR(crop_height);
        return std::nullopt;
    }

    // 裁剪出窗口客户区
    cv::Rect roi(crop_x, crop_y, crop_width, crop_height);
    cv::Mat cropped = img(roi);

    return cropped;
}

RECT DesktopDupWindowScreencap::get_window_client_rect_screen() const
{
    RECT client_rect = { 0 };
    RECT result = { 0 };

    // 获取窗口客户区（相对于窗口）
    if (!GetClientRect(hwnd_, &client_rect)) {
        LogError << "GetClientRect failed, error code: " << GetLastError();
        return result;
    }

    // 将客户区左上角转换为屏幕坐标
    POINT client_top_left = { client_rect.left, client_rect.top };
    if (!ClientToScreen(hwnd_, &client_top_left)) {
        LogError << "ClientToScreen failed, error code: " << GetLastError();
        return result;
    }

    // 计算客户区在屏幕上的位置（只需要转换左上角，然后加上宽高）
    result.left = client_top_left.x;
    result.top = client_top_left.y;
    result.right = client_top_left.x + (client_rect.right - client_rect.left);
    result.bottom = client_top_left.y + (client_rect.bottom - client_rect.top);

    return result;
}

MAA_CTRL_UNIT_NS_END

