#include "BackBufferScreencap.h"

#include "HwndUtils.hpp"
#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

BackBufferScreencap::~BackBufferScreencap()
{
    uninit();
}

std::optional<cv::Mat> BackBufferScreencap::screencap()
{
    if (!dxgi_swap_chain_) {
        if (!init()) {
            LogError << "init failed";
            uninit();
            return std::nullopt;
        }
    }

    HRESULT ret = S_OK;

    ID3D11Texture2D* back_buffer = nullptr;
    ret = dxgi_swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
    if (FAILED(ret)) {
        LogError << "GetBuffer failed" << VAR(ret);
        return std::nullopt;
    }
    OnScopeLeave([&]() {
        if (back_buffer) {
            back_buffer->Release();
            back_buffer = nullptr;
        }
    });

    if (!readable_texture_ && !init_texture(back_buffer)) {
        LogError << "falied to init_texture";
        return std::nullopt;
    }

    d3d_context_->CopyResource(readable_texture_, back_buffer);

    D3D11_MAPPED_SUBRESOURCE mapped { 0 };
    ret = d3d_context_->Map(readable_texture_, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(ret)) {
        LogError << "Map failed" << VAR(ret);
        return std::nullopt;
    }
    OnScopeLeave([&]() { d3d_context_->Unmap(readable_texture_, 0); });

    cv::Mat mat(texture_desc_.Height, texture_desc_.Width, CV_8UC4, mapped.pData, mapped.RowPitch);
    return bgra_to_bgr(mat);
}

bool BackBufferScreencap::init()
{
    LogFunc;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    HRESULT ret = S_OK;

    DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
    swap_chain_desc.BufferCount = 1;
    swap_chain_desc.BufferDesc.Width = 0;  // auto detect
    swap_chain_desc.BufferDesc.Height = 0; // auto detect
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferDesc.RefreshRate.Numerator = 0;
    swap_chain_desc.BufferDesc.RefreshRate.Denominator = 0;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.OutputWindow = hwnd_;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;
    swap_chain_desc.Windowed = is_fullscreen(hwnd_) ? FALSE : TRUE;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

    ret = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        &dxgi_swap_chain_,
        &d3d_device_,
        nullptr,
        &d3d_context_);

    if (FAILED(ret)) {
        LogError << "D3D11CreateDeviceAndSwapChain failed" << VAR(ret);
        return false;
    }

    return true;
}

bool BackBufferScreencap::init_texture(ID3D11Texture2D* raw_texture)
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

void BackBufferScreencap::uninit()
{
    LogFunc;

    if (readable_texture_) {
        readable_texture_->Release();
        readable_texture_ = nullptr;
    }
    texture_desc_ = { 0 };

    if (d3d_context_) {
        d3d_context_->Release();
        d3d_context_ = nullptr;
    }
    if (d3d_device_) {
        d3d_device_->Release();
        d3d_device_ = nullptr;
    }
    if (dxgi_swap_chain_) {
        dxgi_swap_chain_->Release();
        dxgi_swap_chain_ = nullptr;
    }
}

MAA_CTRL_UNIT_NS_END
