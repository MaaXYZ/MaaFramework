#include "DesktopDupWindowScreencap.h"

#include <algorithm>

#include "HwndUtils.hpp"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

DesktopDupWindowScreencap::~DesktopDupWindowScreencap()
{
    uninit();
}

std::optional<cv::Mat> DesktopDupWindowScreencap::screencap()
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return std::nullopt;
    }

    if (!d3d_device_) {
        if (!init()) {
            LogError << "failed to init";
            uninit();
            return std::nullopt;
        }

        // 前几张图片是空的，跳过
        for (int i = 0; i < 3; ++i) {
            auto opt = screencap_impl();
            if (opt) {
                const auto& br = *(opt->end<cv::Vec4b>() - 1);
                if (br[3] == 255) { // only check alpha
                    break;
                }
            }
            LogWarn << "blank image, continue";
        }
    }

    auto img = screencap_impl();
    if (!img) {
        return std::nullopt;
    }

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
    if (client_rect_screen.left >= img->cols || client_rect_screen.top >= img->rows || client_rect_screen.right <= 0
        || client_rect_screen.bottom <= 0) {
        LogError << "Client rect out of bounds" << VAR(client_rect_screen.left) << VAR(client_rect_screen.top) << VAR(img->cols)
                 << VAR(img->rows);
        return std::nullopt;
    }

    // 计算裁剪区域（确保在图像范围内）
    int crop_x = std::max(0, static_cast<int>(client_rect_screen.left));
    int crop_y = std::max(0, static_cast<int>(client_rect_screen.top));
    int crop_width = std::min(client_width, img->cols - crop_x);
    int crop_height = std::min(client_height, img->rows - crop_y);

    if (crop_width <= 0 || crop_height <= 0) {
        LogError << "Invalid crop region" << VAR(crop_x) << VAR(crop_y) << VAR(crop_width) << VAR(crop_height);
        return std::nullopt;
    }

    // 裁剪出窗口客户区
    cv::Rect roi(crop_x, crop_y, crop_width, crop_height);
    cv::Mat cropped = (*img)(roi);

    // 如果裁剪区域小于客户区，可能需要处理边界情况
    // 这里简单返回裁剪后的图像
    return bgra_to_bgr(cropped);
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

bool DesktopDupWindowScreencap::init()
{
    LogFunc;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    HRESULT ret = S_OK;

    ret = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &d3d_device_,
        nullptr,
        &d3d_context_);
    if (FAILED(ret)) {
        LogError << "D3D11CreateDevice failed" << VAR(ret);
        return false;
    }

    ret = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgi_factory_));
    if (FAILED(ret)) {
        LogError << "CreateDXGIFactory failed" << VAR(ret);
        return false;
    }

    ret = dxgi_factory_->EnumAdapters(0, &dxgi_adapter_);
    if (FAILED(ret)) {
        LogError << "EnumAdapters failed" << VAR(ret);
        return false;
    }

    ret = dxgi_adapter_->EnumOutputs(0, reinterpret_cast<IDXGIOutput**>(&dxgi_output_));
    if (FAILED(ret)) {
        LogError << "EnumOutputs failed" << VAR(ret);
        return false;
    }

    ret = dxgi_output_->DuplicateOutput(d3d_device_, &dxgi_dup_);
    if (FAILED(ret)) {
        LogError << "DuplicateOutput failed" << VAR(ret);
        return false;
    }

    return true;
}

bool DesktopDupWindowScreencap::init_texture(ID3D11Texture2D* raw_texture)
{
    LogFunc;

    if (!d3d_device_ || !raw_texture) {
        LogError << "handle is null" << VAR_VOIDP(d3d_device_) << VAR_VOIDP(raw_texture);
        return false;
    }

    raw_texture->GetDesc(&texture_desc_);

    texture_desc_.BindFlags = 0;
    texture_desc_.MiscFlags = 0;
    texture_desc_.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    texture_desc_.Usage = D3D11_USAGE_STAGING;

    HRESULT ret = d3d_device_->CreateTexture2D(&texture_desc_, NULL, &readable_texture_);
    if (FAILED(ret)) {
        LogError << "CreateTexture2D failed" << VAR(ret);
        return false;
    }

    return true;
}

void DesktopDupWindowScreencap::uninit()
{
    LogFunc;

    if (readable_texture_) {
        readable_texture_->Release();
        readable_texture_ = nullptr;
    }
    texture_desc_ = { 0 };

    if (dxgi_dup_) {
        dxgi_dup_->Release();
        dxgi_dup_ = nullptr;
    }
    if (dxgi_output_) {
        dxgi_output_->Release();
        dxgi_output_ = nullptr;
    }
    if (dxgi_adapter_) {
        dxgi_adapter_->Release();
        dxgi_adapter_ = nullptr;
    }
    if (dxgi_factory_) {
        dxgi_factory_->Release();
        dxgi_factory_ = nullptr;
    }
    if (d3d_context_) {
        d3d_context_->Release();
        d3d_context_ = nullptr;
    }
    if (d3d_device_) {
        d3d_device_->Release();
        d3d_device_ = nullptr;
    }
}

std::optional<cv::Mat> DesktopDupWindowScreencap::screencap_impl()
{
    if (!d3d_context_ || !dxgi_dup_) {
        LogError << "handle is null" << VAR_VOIDP(d3d_context_) << VAR_VOIDP(dxgi_dup_);
        return std::nullopt;
    }

    HRESULT ret = S_OK;

    DXGI_OUTDUPL_FRAME_INFO frame_info { 0 };
    IDXGIResource* desktop_resource = nullptr;
    ret = dxgi_dup_->AcquireNextFrame(INFINITE, &frame_info, &desktop_resource);
    if (FAILED(ret)) {
        LogError << "AcquireNextFrame failed" << VAR(ret);
        return std::nullopt;
    }
    OnScopeLeave([&]() {
        dxgi_dup_->ReleaseFrame();

        if (desktop_resource) {
            desktop_resource->Release();
            desktop_resource = nullptr;
        }
    });

    ID3D11Texture2D* raw_texture = nullptr;
    ret = desktop_resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&raw_texture));
    if (FAILED(ret)) {
        LogError << "QueryInterface ID3D11Texture2D failed" << VAR(ret);
        return std::nullopt;
    }
    OnScopeLeave([&]() {
        if (raw_texture) {
            raw_texture->Release();
            raw_texture = nullptr;
        }
    });

    if (!readable_texture_ && !init_texture(raw_texture)) {
        LogError << "failed to init_texture";
        return std::nullopt;
    }

    d3d_context_->CopyResource(readable_texture_, raw_texture);

    D3D11_MAPPED_SUBRESOURCE mapped { 0 };
    ret = d3d_context_->Map(readable_texture_, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(ret)) {
        LogError << "Map failed" << VAR(ret);
        return std::nullopt;
    }
    OnScopeLeave([&]() { d3d_context_->Unmap(readable_texture_, 0); });

    cv::Mat mat(texture_desc_.Height, texture_desc_.Width, CV_8UC4, mapped.pData, mapped.RowPitch);
    return mat;
}

MAA_CTRL_UNIT_NS_END

