#include "PrintWindowScreencap.h"

#include "HwndUtils.hpp"
#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

std::optional<cv::Mat> PrintWindowScreencap::screencap()
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return std::nullopt;
    }

    auto image = capture_window(hwnd_);
    if (!image) {
        return std::nullopt;
    }

    HWND root = GetAncestor(hwnd_, GA_ROOTOWNER);
    if (!root) {
        root = hwnd_;
    }

    HWND popup = GetLastActivePopup(root);
    if (!popup || popup == hwnd_ || popup == root || !IsWindow(popup) || !IsWindowVisible(popup)) {
        return image;
    }

    auto popup_image = capture_window(popup);
    if (!popup_image) {
        LogWarn << "Failed to capture active popup, returning main window image" << VAR(popup);
        return image;
    }

    POINT main_origin = { 0, 0 };
    POINT popup_origin = { 0, 0 };
    if (!ClientToScreen(hwnd_, &main_origin) || !ClientToScreen(popup, &popup_origin)) {
        LogWarn << "Failed to locate active popup, returning main window image" << VAR(popup) << VAR(GetLastError());
        return image;
    }

    const int offset_x = popup_origin.x - main_origin.x;
    const int offset_y = popup_origin.y - main_origin.y;
    const cv::Rect canvas_rect(0, 0, image->cols, image->rows);
    const cv::Rect popup_rect(offset_x, offset_y, popup_image->cols, popup_image->rows);
    const cv::Rect dst_rect = canvas_rect & popup_rect;
    if (dst_rect.empty()) {
        LogWarn << "Active popup is outside the main client area" << VAR(popup) << VAR(offset_x) << VAR(offset_y);
        return image;
    }

    const cv::Rect src_rect(dst_rect.x - offset_x, dst_rect.y - offset_y, dst_rect.width, dst_rect.height);
    (*popup_image)(src_rect).copyTo((*image)(dst_rect));
    return image;
}

std::optional<cv::Mat> PrintWindowScreencap::capture_window(HWND capture_hwnd)
{
    if (!capture_hwnd || !IsWindow(capture_hwnd)) {
        LogError << "Invalid capture window" << VAR(capture_hwnd);
        return std::nullopt;
    }

    // 确定要捕获的区域大小
    // 使用PW_CLIENTONLY标志，只获取客户端区域（不含窗口边框）
    RECT rect = { 0 };
    if (!GetClientRect(capture_hwnd, &rect)) {
        LogError << "GetClientRect failed, error code: " << GetLastError();
        return std::nullopt;
    }

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    if (width <= 0 || height <= 0) {
        LogError << "Invalid window size" << VAR(width) << VAR(height);
        return std::nullopt;
    }

    HDC hdc = nullptr;
    HDC mem_dc = nullptr;
    HBITMAP bitmap = nullptr;
    HGDIOBJ old_obj = nullptr;

    OnScopeLeave([&]() {
        if (old_obj) {
            SelectObject(mem_dc, old_obj);
        }
        if (bitmap) {
            DeleteObject(bitmap);
        }
        if (mem_dc) {
            DeleteDC(mem_dc);
        }
        if (hdc) {
            ReleaseDC(capture_hwnd, hdc);
        }
    });

    // 创建与窗口兼容的 DC
    hdc = GetDC(capture_hwnd);
    if (!hdc) {
        LogError << "GetDC failed, error code: " << GetLastError();
        return std::nullopt;
    }

    mem_dc = CreateCompatibleDC(hdc);
    if (!mem_dc) {
        LogError << "CreateCompatibleDC failed, error code: " << GetLastError();
        return std::nullopt;
    }

    bitmap = CreateCompatibleBitmap(hdc, width, height);
    if (!bitmap) {
        LogError << "CreateCompatibleBitmap failed, error code: " << GetLastError();
        return std::nullopt;
    }

    old_obj = SelectObject(mem_dc, bitmap);
    if (!old_obj) {
        LogError << "SelectObject failed, error code: " << GetLastError();
        return std::nullopt;
    }

    // 使用PrintWindow捕获窗口内容
    // 使用PW_CLIENTONLY | PW_RENDERFULLCONTENT标志:
    // - PW_CLIENTONLY (0x1): 只获取客户端区域
    // - PW_RENDERFULLCONTENT (0x2): 捕获非最小化后台窗口
    constexpr UINT nFlags = PW_CLIENTONLY | PW_RENDERFULLCONTENT;
    if (!PrintWindow(capture_hwnd, mem_dc, nFlags)) {
        LogError << "PrintWindow failed, error code: " << GetLastError();
        return std::nullopt;
    }

    // 使用 GetDIBits 将位图一致转换为 32bpp BGRA
    BITMAPINFO bmi = { };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    cv::Mat mat(height, width, CV_8UC4);
    if (!GetDIBits(mem_dc, bitmap, 0, height, mat.data, &bmi, DIB_RGB_COLORS)) {
        LogError << "GetDIBits failed, error code: " << GetLastError();
        return std::nullopt;
    }

    return bgra_to_bgr(mat);
}

MAA_CTRL_UNIT_NS_END
