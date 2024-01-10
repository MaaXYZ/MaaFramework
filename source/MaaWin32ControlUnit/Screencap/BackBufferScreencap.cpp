#include "BackBufferScreencap.h"

#include "HwndUtils.hpp"
#include "Utils/ImageIo.h"
#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

BackBufferScreencap::~BackBufferScreencap()
{
    uninit();
}

std::optional<cv::Mat> BackBufferScreencap::screencap()
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return std::nullopt;
    }

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

    d3d_context_->CopyResource(readable_texture_, readable_texture_);

    D3D11_MAPPED_SUBRESOURCE mapped { 0 };
    ret = d3d_context_->Map(readable_texture_, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(ret)) {
        LogError << "Map failed" << VAR(ret);
        return std::nullopt;
    }
    OnScopeLeave([&]() { d3d_context_->Unmap(readable_texture_, 0); });

    cv::Mat mat(texture_desc_.Height, texture_desc_.Width, CV_8UC4, mapped.pData, mapped.RowPitch);

    return mat.clone();
}

bool BackBufferScreencap::init()
{
    LogFunc;

    HRESULT ret = S_OK;

    DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
    swap_chain_desc.BufferCount = 1;
    swap_chain_desc.BufferDesc.Width = 0;  // auto detect
    swap_chain_desc.BufferDesc.Height = 0; // auto detect
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    // sd.BufferDesc.RefreshRate.Numerator = 60;
    // sd.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_desc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
    swap_chain_desc.OutputWindow = hwnd_;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;
    swap_chain_desc.Windowed = is_fullscreen(hwnd_) ? FALSE : TRUE;

    ret = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                        &swap_chain_desc, &dxgi_swap_chain_, &d3d_device_, nullptr, &d3d_context_);

    if (FAILED(ret)) {
        LogError << "D3D11CreateDeviceAndSwapChain failed" << VAR(ret);
        return false;
    }
    dxgi_swap_chain_->GetDesc(&swap_chain_desc);

    texture_desc_.Width = swap_chain_desc.BufferDesc.Width;
    texture_desc_.Height = swap_chain_desc.BufferDesc.Height;
    texture_desc_.Format = swap_chain_desc.BufferDesc.Format;
    texture_desc_.SampleDesc = swap_chain_desc.SampleDesc;
    texture_desc_.MipLevels = 1;
    texture_desc_.ArraySize = 1;
    texture_desc_.Usage = D3D11_USAGE_STAGING;
    texture_desc_.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ret = d3d_device_->CreateTexture2D(&texture_desc_, nullptr, &readable_texture_);
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
