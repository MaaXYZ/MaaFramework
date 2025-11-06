#include "MagnificationScreencap.h"

#include <magnification.h>

#include "HwndUtils.hpp"
#include "MaaUtils/Logger.h"

#pragma comment(lib, "magnification.lib")

MAA_CTRL_UNIT_NS_BEGIN

// 放大镜窗口类名
constexpr const wchar_t* kMagnifierWindowClass = L"MaaMagnifierWindow";

// 放大镜窗口过程
LRESULT CALLBACK MagnifierWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

MagnificationScreencap::~MagnificationScreencap()
{
    uninit();
}

std::optional<cv::Mat> MagnificationScreencap::screencap()
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return std::nullopt;
    }

    if (!initialized_) {
        if (!init()) {
            LogError << "init failed";
            return std::nullopt;
        }
    }

    // 获取窗口客户区大小和位置
    RECT client_rect = { 0 };
    if (!GetClientRect(hwnd_, &client_rect)) {
        LogError << "GetClientRect failed, error code: " << GetLastError();
        return std::nullopt;
    }

    int width = client_rect.right - client_rect.left;
    int height = client_rect.bottom - client_rect.top;

    if (width <= 0 || height <= 0) {
        LogError << "Invalid window size" << VAR(width) << VAR(height);
        return std::nullopt;
    }

    // 计算客户区在屏幕上的位置
    POINT client_top_left = { client_rect.left, client_rect.top };
    if (!ClientToScreen(hwnd_, &client_top_left)) {
        LogError << "ClientToScreen failed, error code: " << GetLastError();
        return std::nullopt;
    }

    // 设置放大镜窗口的位置和大小（与客户区相同）
    if (!SetWindowPos(mag_window_, nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_HIDEWINDOW)) {
        LogError << "SetWindowPos failed, error code: " << GetLastError();
        return std::nullopt;
    }

    // 设置放大镜的源区域（要截图的窗口客户区）
    RECT source_rect = { client_top_left.x, client_top_left.y, client_top_left.x + width, client_top_left.y + height };

    if (!MagSetWindowSource(mag_window_, source_rect)) {
        LogError << "MagSetWindowSource failed, error code: " << GetLastError();
        return std::nullopt;
    }

    // 设置变换矩阵为1:1（不放大，原样显示）
    // 矩阵格式: [M11, M12, M21, M22, Dx, Dy]
    // M11和M22控制缩放，设置为1.0表示不缩放
    MAGTRANSFORM matrix = { 0 };
    matrix.v[0][0] = 1.0f; // M11 - X轴缩放
    matrix.v[0][1] = 0.0f; // M12
    matrix.v[0][2] = 0.0f; // Dx - X轴偏移
    matrix.v[1][0] = 0.0f; // M21
    matrix.v[1][1] = 1.0f; // M22 - Y轴缩放
    matrix.v[1][2] = 0.0f; // Dy - Y轴偏移
    matrix.v[2][0] = 0.0f;
    matrix.v[2][1] = 0.0f;
    matrix.v[2][2] = 1.0f;

    if (!MagSetWindowTransform(mag_window_, &matrix)) {
        LogError << "MagSetWindowTransform failed, error code: " << GetLastError();
        return std::nullopt;
    }

    // 显示放大镜窗口（虽然用户看不到，但需要显示才能渲染内容）
    ShowWindow(mag_window_, SW_SHOW);
    UpdateWindow(mag_window_);

    // 等待一小段时间让放大镜窗口渲染内容
    Sleep(10);

    // 从放大镜窗口获取DC并复制内容
    HDC mag_dc = nullptr;
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
        if (mag_dc) {
            ReleaseDC(mag_window_, mag_dc);
        }
        // 隐藏放大镜窗口
        ShowWindow(mag_window_, SW_HIDE);
    });

    mag_dc = GetDC(mag_window_);
    if (!mag_dc) {
        LogError << "GetDC failed, error code: " << GetLastError();
        return std::nullopt;
    }

    mem_dc = CreateCompatibleDC(mag_dc);
    if (!mem_dc) {
        LogError << "CreateCompatibleDC failed, error code: " << GetLastError();
        return std::nullopt;
    }

    bitmap = CreateCompatibleBitmap(mag_dc, width, height);
    if (!bitmap) {
        LogError << "CreateCompatibleBitmap failed, error code: " << GetLastError();
        return std::nullopt;
    }

    old_obj = SelectObject(mem_dc, bitmap);
    if (!old_obj) {
        LogError << "SelectObject failed, error code: " << GetLastError();
        return std::nullopt;
    }

    // 从放大镜窗口复制内容到内存DC
    if (!BitBlt(mem_dc, 0, 0, width, height, mag_dc, 0, 0, SRCCOPY)) {
        LogError << "BitBlt failed, error code: " << GetLastError();
        return std::nullopt;
    }

    cv::Mat mat(height, width, CV_8UC4);
    if (!GetBitmapBits(bitmap, width * height * 4, mat.data)) {
        LogError << "GetBitmapBits failed, error code: " << GetLastError();
        return std::nullopt;
    }

    return bgra_to_bgr(mat);
}

bool MagnificationScreencap::init()
{
    LogFunc;

    if (initialized_) {
        return true;
    }

    // 初始化放大镜库
    if (!MagInitialize()) {
        LogError << "MagInitialize failed, error code: " << GetLastError();
        return false;
    }

    // 注册放大镜窗口类（如果还没有注册）
    static bool class_registered = false;
    if (!class_registered) {
        WNDCLASSEXW wc = { 0 };
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = MagnifierWndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = kMagnifierWindowClass;
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

        if (!RegisterClassExW(&wc)) {
            DWORD error = GetLastError();
            if (error != ERROR_CLASS_ALREADY_EXISTS) {
                LogError << "RegisterClassExW failed, error code: " << error;
                MagUninitialize();
                return false;
            }
        }
        class_registered = true;
    }

    // 创建放大镜窗口（初始位置在屏幕外，大小1x1）
    mag_window_ = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        kMagnifierWindowClass,
        L"MaaMagnifier",
        WS_POPUP,
        -32000,
        -32000, // 放在屏幕外
        1,
        1,      // 初始大小1x1
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr);

    if (!mag_window_) {
        LogError << "CreateWindowExW failed, error code: " << GetLastError();
        MagUninitialize();
        return false;
    }

    // 设置窗口为放大镜窗口
    if (!MagSetWindowFilterList(mag_window_, MW_FILTERMODE_EXCLUDE, 0, nullptr)) {
        // 这个调用可能失败，但不影响基本功能，只记录警告
        LogWarn << "MagSetWindowFilterList failed, error code: " << GetLastError();
    }

    initialized_ = true;
    return true;
}

void MagnificationScreencap::uninit()
{
    if (mag_window_) {
        DestroyWindow(mag_window_);
        mag_window_ = nullptr;
    }

    if (initialized_) {
        MagUninitialize();
        initialized_ = false;
    }
}

MAA_CTRL_UNIT_NS_END

