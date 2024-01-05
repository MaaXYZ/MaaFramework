#include "GdiScreencap.h"

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

std::optional<cv::Mat> GdiScreencap::screencap()
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return std::nullopt;
    }

    RECT rect { 0 };
    if (!GetClientRect(hwnd_, &rect)) {
        LogError << "GetClientRect failed, error code: " << GetLastError();
        return std::nullopt;
    }

    double screen_scale = window_screen_scale();

    int width = static_cast<int>(screen_scale * (rect.right - rect.left));
    int height = static_cast<int>(screen_scale * (rect.bottom - rect.top));

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
            ReleaseDC(hwnd_, hdc);
        }
    });

    hdc = GetDC(hwnd_);
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

    if (!BitBlt(mem_dc, 0, 0, width, height, hdc, 0, 0, SRCCOPY)) {
        LogError << "BitBlt failed, error code: " << GetLastError();
        return std::nullopt;
    }

    cv::Mat mat(height, width, CV_8UC4);
    if (!GetBitmapBits(bitmap, width * height * 4, mat.data)) {
        LogError << "GetBitmapBits failed, error code: " << GetLastError();
        return std::nullopt;
    }

    return mat;
}

double GdiScreencap::window_screen_scale()
{
#ifndef MAA_WIN32_COMPATIBLE

    constexpr double kStandardDPI = 96.0;
    // 运行期需要 Win10 1607 以上版本
    return GetDpiForWindow(hwnd_) / kStandardDPI;

#else

    HMONITOR monitor_handle = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);

    MONITORINFOEX miex { 0 };
    miex.cbSize = sizeof(miex);
    GetMonitorInfo(monitor_handle, &miex);
    LONG screen_x_logical = miex.rcMonitor.right - miex.rcMonitor.left;

    DEVMODE dm { 0 };
    dm.dmSize = sizeof(dm);
    EnumDisplaySettings(miex.szDevice, ENUM_CURRENT_SETTINGS, &dm);
    DWORD screen_x_physical = dm.dmPelsWidth;

    return static_cast<double>(screen_x_physical) / static_cast<double>(screen_x_logical);

#endif
}

MAA_CTRL_UNIT_NS_END
