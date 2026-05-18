#include "PrintWindowScreencap.h"

#include <chrono>

#include "HdrDisplayUtils.hpp"
#include "HwndUtils.hpp"
#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

std::optional<cv::Mat> PrintWindowScreencap::screencap()
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        last_screencap_info_ = {};
        return std::nullopt;
    }

    // 确定要捕获的区域大小
    // 使用PW_CLIENTONLY标志，只获取客户端区域（不含窗口边框）
    RECT rect = { 0 };
    if (!GetClientRect(hwnd_, &rect)) {
        LogError << "GetClientRect failed, error code: " << GetLastError();
        last_screencap_info_ = {};
        return std::nullopt;
    }

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    if (width <= 0 || height <= 0) {
        LogError << "Invalid window size" << VAR(width) << VAR(height);
        last_screencap_info_ = {};
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
            ReleaseDC(hwnd_, hdc);
        }
    });

    // 创建与窗口兼容的 DC
    hdc = GetDC(hwnd_);
    if (!hdc) {
        LogError << "GetDC failed, error code: " << GetLastError();
        last_screencap_info_ = {};
        return std::nullopt;
    }

    mem_dc = CreateCompatibleDC(hdc);
    if (!mem_dc) {
        LogError << "CreateCompatibleDC failed, error code: " << GetLastError();
        last_screencap_info_ = {};
        return std::nullopt;
    }

    bitmap = CreateCompatibleBitmap(hdc, width, height);
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

    // 使用PrintWindow捕获窗口内容
    // 使用PW_CLIENTONLY | PW_RENDERFULLCONTENT标志:
    // - PW_CLIENTONLY (0x1): 只获取客户端区域
    // - PW_RENDERFULLCONTENT (0x2): 捕获非最小化后台窗口
    constexpr UINT nFlags = PW_CLIENTONLY | PW_RENDERFULLCONTENT;
    if (!PrintWindow(hwnd_, mem_dc, nFlags)) {
        LogError << "PrintWindow failed, error code: " << GetLastError();
        last_screencap_info_ = {};
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
                LogInfo << "PrintWindow HDR display compensation applied" << VAR(hdr_state->sdr_white_nits);
            }
            return compensated;
        }
    }

    last_screencap_info_ = {};
    return bgra_to_bgr(mat);
}

MAA_CTRL_UNIT_NS_END
