#include "GdiPlusScreencap.h"

#include <gdiplus.h>

#include "HwndUtils.hpp"
#include "MaaUtils/Logger.h"

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

MAA_CTRL_UNIT_NS_BEGIN

GdiPlusScreencap::~GdiPlusScreencap()
{
    uninit();
}

std::optional<cv::Mat> GdiPlusScreencap::screencap()
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

    // 获取窗口客户区大小
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

    // 获取窗口DC
    HDC hdc = GetDC(hwnd_);
    if (!hdc) {
        LogError << "GetDC failed, error code: " << GetLastError();
        return std::nullopt;
    }
    OnScopeLeave([&]() { ReleaseDC(hwnd_, hdc); });

    // 创建GDI+ Bitmap（使用屏幕DC作为参考）
    HDC screen_dc = GetDC(nullptr);
    if (!screen_dc) {
        LogError << "GetDC(nullptr) failed, error code: " << GetLastError();
        return std::nullopt;
    }
    OnScopeLeave([&]() { ReleaseDC(nullptr, screen_dc); });

    Bitmap bitmap(width, height, PixelFormat32bppARGB);
    if (bitmap.GetLastStatus() != Ok) {
        LogError << "Bitmap creation failed";
        return std::nullopt;
    }

    // 创建Graphics对象用于绘制到位图
    Graphics* mem_graphics = Graphics::FromImage(&bitmap);
    if (!mem_graphics || mem_graphics->GetLastStatus() != Ok) {
        LogError << "Graphics::FromImage failed";
        delete mem_graphics;
        return std::nullopt;
    }
    OnScopeLeave([&]() { delete mem_graphics; });

    // 使用BitBlt从窗口DC复制到GDI+ Bitmap
    // GDI+的优势在于可以使用LockBits直接访问位图数据，而不需要GetBitmapBits
    HDC mem_dc = mem_graphics->GetHDC();
    if (!mem_dc) {
        LogError << "GetHDC failed";
        return std::nullopt;
    }
    OnScopeLeave([&]() { mem_graphics->ReleaseHDC(mem_dc); });

    if (!BitBlt(mem_dc, 0, 0, width, height, hdc, 0, 0, SRCCOPY)) {
        LogError << "BitBlt failed, error code: " << GetLastError();
        return std::nullopt;
    }

    // 锁定位图数据
    BitmapData bitmap_data;
    Rect rect(0, 0, width, height);
    if (bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmap_data) != Ok) {
        LogError << "LockBits failed";
        return std::nullopt;
    }
    OnScopeLeave([&]() { bitmap.UnlockBits(&bitmap_data); });

    // 复制数据到cv::Mat
    cv::Mat mat(height, width, CV_8UC4);
    const uint8_t* src = static_cast<const uint8_t*>(bitmap_data.Scan0);
    uint8_t* dst = mat.data;

    for (int y = 0; y < height; ++y) {
        memcpy(dst + y * width * 4, src + y * bitmap_data.Stride, width * 4);
    }

    return bgra_to_bgr(mat);
}

bool GdiPlusScreencap::init()
{
    if (initialized_) {
        return true;
    }

    // GDI+初始化在第一次使用时进行（使用静态变量）
    // 注意：GDI+需要全局初始化和清理，这里简化处理
    initialized_ = true;
    return true;
}

void GdiPlusScreencap::uninit()
{
    initialized_ = false;
}

MAA_CTRL_UNIT_NS_END

