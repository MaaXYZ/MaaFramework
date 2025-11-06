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

    // 确定要捕获的区域大小
    RECT rect = { 0 };
    bool use_client_rect = (nFlags_ & PW_CLIENTONLY) != 0;

    if (use_client_rect) {
        // 只获取客户端区域（不含窗口边框）
        if (!GetClientRect(hwnd_, &rect)) {
            LogError << "GetClientRect failed, error code: " << GetLastError();
            return std::nullopt;
        }
    }
    else {
        // 获取整个窗口区域
        if (!GetWindowRect(hwnd_, &rect)) {
            LogError << "GetWindowRect failed, error code: " << GetLastError();
            return std::nullopt;
        }
        // 转换为相对于窗口的坐标
        rect.right -= rect.left;
        rect.bottom -= rect.top;
        rect.left = 0;
        rect.top = 0;
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
            ReleaseDC(nullptr, hdc);
        }
    });

    // 创建内存DC
    hdc = GetDC(nullptr);
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
    // nFlags可以包含:
    // - PW_CLIENTONLY (0x1): 只获取客户端区域
    // - PW_RENDERFULLCONTENT (0x2): 捕获非最小化后台窗口
    if (!PrintWindow(hwnd_, mem_dc, nFlags_)) {
        LogError << "PrintWindow failed, error code: " << GetLastError();
        return std::nullopt;
    }

    cv::Mat mat(height, width, CV_8UC4);
    if (!GetBitmapBits(bitmap, width * height * 4, mat.data)) {
        LogError << "GetBitmapBits failed, error code: " << GetLastError();
        return std::nullopt;
    }

    return bgra_to_bgr(mat);
}

MAA_CTRL_UNIT_NS_END

