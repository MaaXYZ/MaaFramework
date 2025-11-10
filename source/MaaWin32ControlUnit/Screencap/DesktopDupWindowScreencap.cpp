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

    // 获取窗口客户区在屏幕上的位置（相对于整个虚拟桌面）
    RECT client_rect_screen = get_window_client_rect_screen();
    if (client_rect_screen.right <= client_rect_screen.left || client_rect_screen.bottom <= client_rect_screen.top) {
        LogError << "Invalid client rect" << VAR(client_rect_screen.left) << VAR(client_rect_screen.top) << VAR(client_rect_screen.right)
                 << VAR(client_rect_screen.bottom);
        return std::nullopt;
    }

    // 获取当前输出（显示器）的桌面坐标
    RECT output_desktop = get_output_desktop_coordinates();
    if (output_desktop.right <= output_desktop.left || output_desktop.bottom <= output_desktop.top) {
        LogError << "Failed to get output desktop coordinates";
        return std::nullopt;
    }

    // 将窗口坐标转换为相对于该显示器的坐标
    int client_width = client_rect_screen.right - client_rect_screen.left;
    int client_height = client_rect_screen.bottom - client_rect_screen.top;
    int crop_x = client_rect_screen.left - output_desktop.left;
    int crop_y = client_rect_screen.top - output_desktop.top;

    // 检查裁剪区域是否在图像范围内
    if (crop_x < 0 || crop_y < 0 || crop_x + client_width > img.cols || crop_y + client_height > img.rows) {
        LogError << "Client rect out of bounds" << VAR(crop_x) << VAR(crop_y) << VAR(client_width) << VAR(client_height) << VAR(img.cols)
                 << VAR(img.rows) << VAR(output_desktop.left) << VAR(output_desktop.top);
        return std::nullopt;
    }

    // 裁剪出窗口客户区
    cv::Rect roi(crop_x, crop_y, client_width, client_height);
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

    // 将客户区左上角转换为屏幕坐标（相对于整个虚拟桌面）
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

RECT DesktopDupWindowScreencap::get_output_desktop_coordinates() const
{
    RECT result = { 0 };

    if (!dxgi_output_) {
        LogError << "dxgi_output_ is nullptr";
        return result;
    }

    DXGI_OUTPUT_DESC output_desc;
    HRESULT ret = dxgi_output_->GetDesc(&output_desc);
    if (FAILED(ret)) {
        LogError << "GetDesc failed" << VAR(ret);
        return result;
    }

    // DesktopCoordinates 是显示器在虚拟桌面中的位置
    result = output_desc.DesktopCoordinates;
    return result;
}

MAA_CTRL_UNIT_NS_END

