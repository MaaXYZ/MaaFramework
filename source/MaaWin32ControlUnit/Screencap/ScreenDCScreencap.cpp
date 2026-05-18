#include "ScreenDCScreencap.h"

#include <chrono>

#include "HdrDisplayUtils.hpp"
#include "HwndUtils.hpp"
#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

std::optional<cv::Mat> ScreenDCScreencap::screencap()
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        last_screencap_info_ = {};
        return std::nullopt;
    }

    // Ensure the window is fully visible on the monitor before screencap
    if (!ensure_window_on_screen(hwnd_)) {
        LogWarn << "Failed to ensure window on screen";
    }

    // 获取窗口在屏幕上的位置
    RECT window_rect = { 0 };
    if (!GetWindowRect(hwnd_, &window_rect)) {
        LogError << "GetWindowRect failed, error code: " << GetLastError();
        last_screencap_info_ = {};
        return std::nullopt;
    }

    // 获取客户区大小
    RECT client_rect = { 0 };
    if (!GetClientRect(hwnd_, &client_rect)) {
        LogError << "GetClientRect failed, error code: " << GetLastError();
        last_screencap_info_ = {};
        return std::nullopt;
    }

    // 计算客户区在屏幕上的位置
    POINT client_top_left = { client_rect.left, client_rect.top };
    if (!ClientToScreen(hwnd_, &client_top_left)) {
        LogError << "ClientToScreen failed, error code: " << GetLastError();
        last_screencap_info_ = {};
        return std::nullopt;
    }

    int width = client_rect.right - client_rect.left;
    int height = client_rect.bottom - client_rect.top;

    if (width <= 0 || height <= 0) {
        LogError << "Invalid window size" << VAR(width) << VAR(height);
        last_screencap_info_ = {};
        return std::nullopt;
    }

    HDC screen_dc = nullptr;
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
        if (screen_dc) {
            ReleaseDC(nullptr, screen_dc);
        }
    });

    // 获取屏幕DC
    screen_dc = GetDC(nullptr);
    if (!screen_dc) {
        LogError << "GetDC failed, error code: " << GetLastError();
        last_screencap_info_ = {};
        return std::nullopt;
    }

    mem_dc = CreateCompatibleDC(screen_dc);
    if (!mem_dc) {
        LogError << "CreateCompatibleDC failed, error code: " << GetLastError();
        last_screencap_info_ = {};
        return std::nullopt;
    }

    bitmap = CreateCompatibleBitmap(screen_dc, width, height);
    if (!bitmap) {
        LogError << "CreateCompatibleBitmap failed, error code: " << GetLastError();
        last_screencap_info_ = {};
        return std::nullopt;
    }

    old_obj = SelectObject(mem_dc, bitmap);
    if (!old_obj) {
        LogError << "SelectObject failed, error code: " << GetLastError();
        last_screencap_info_ = {};
        return std::nullopt;
    }

    // 从屏幕DC复制客户区内容
    if (!BitBlt(mem_dc, 0, 0, width, height, screen_dc, client_top_left.x, client_top_left.y, SRCCOPY)) {
        LogError << "BitBlt failed, error code: " << GetLastError();
        last_screencap_info_ = {};
        return std::nullopt;
    }

    cv::Mat mat(height, width, CV_8UC4);
    if (!GetBitmapBits(bitmap, width * height * 4, mat.data)) {
        LogError << "GetBitmapBits failed, error code: " << GetLastError();
        last_screencap_info_ = {};
        return std::nullopt;
    }

    const HMONITOR target_monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
    const std::optional<HdrDisplayState> hdr_state = query_hdr_display_state(target_monitor);
    if (hdr_state.has_value() && hdr_state->valid && hdr_state->hdr_enabled) {
        cv::Mat compensated = compensate_hdr_sdr_capture(mat, hdr_state->sdr_white_nits);
        if (!compensated.empty()) {
            last_screencap_info_ = {
                .hdr_capture_active = false,
                .hdr_preprocessed = true,
                .gpu_processed = false,
                .display_hdr_active = true,
                .display_hdr_compensated = true,
            };
            static auto last_log_time = std::chrono::steady_clock::time_point {};
            const auto now = std::chrono::steady_clock::now();
            if (last_log_time == std::chrono::steady_clock::time_point {} || now - last_log_time > std::chrono::seconds(5)) {
                last_log_time = now;
                LogInfo << "ScreenDC HDR display compensation applied" << VAR(hdr_state->sdr_white_nits);
            }
            return compensated;
        }
    }

    last_screencap_info_ = {};
    return bgra_to_bgr(mat);
}

MAA_CTRL_UNIT_NS_END
