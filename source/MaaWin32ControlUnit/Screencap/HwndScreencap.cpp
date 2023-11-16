#include "HwndScreencap.h"

#include <functional>

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

double get_window_screen_scale()
{
#ifdef _WIN32_WINNT_WIN10
    //需要win10 1607以上版本
    double screen_scale = GetDpiForWindow(GetDesktopWindow()) / 96.0;
#else
    HWND desktop_hwnd = GetDesktopWindow();
    HMONITOR monitor_handle = MonitorFromWindow(desktop_hwnd, MONITOR_DEFAULTTONEAREST);

    MONITORINFOEX miex;
    miex.cbSize = sizeof(miex);
    GetMonitorInfo(monitor_handle, &miex);
    int screen_x_logical = (miex.rcMonitor.right - miex.rcMonitor.left);

    DEVMODE dm;
    dm.dmSize = sizeof(dm);
    dm.dmDriverExtra = 0;
    EnumDisplaySettings(miex.szDevice, ENUM_CURRENT_SETTINGS, &dm);
    int screen_x_physical = dm.dmPelsWidth;

    double screen_scale = ((double)screen_x_physical / (double)screen_x_logical);
#endif
    return screen_scale;
}

std::optional<cv::Mat> HwndScreencap::screencap()
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return std::nullopt;
    }

    RECT rect;
    if (!GetClientRect(hwnd_, &rect)) {
        LogError << "GetClientRect failed, error code: " << GetLastError();
        return std::nullopt;
    }

    double screen_scale = get_window_screen_scale();

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

MAA_CTRL_UNIT_NS_END
