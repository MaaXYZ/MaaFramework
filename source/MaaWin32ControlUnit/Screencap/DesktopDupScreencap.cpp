#include "DesktopDupScreencap.h"

#include "HwndUtils.hpp"
#include "Utils/ImageIo.h"
#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

DesktopDupScreencap::~DesktopDupScreencap()
{
    uninit();
}

std::optional<cv::Mat> DesktopDupScreencap::screencap()
{
    if (!d3d_device_) {
        if (!init()) {
            LogError << "falied to init";
            uninit();
            return std::nullopt;
        }

        // 前几张图片是空的
        while (auto opt = screencap_impl()) {
            const auto& br = *(opt->end<cv::Vec4b>() - 1);
            if (br[3] == 255) { // only check alpha
                break;
            }
            LogWarn << "blank image, continue";
        }
    }

    auto img = screencap_impl();
    if (!img) {
        return std::nullopt;
    }
    return bgra_to_bgr(*img);
}

bool DesktopDupScreencap::init()
{
    LogFunc;

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

bool DesktopDupScreencap::init_texture(ID3D11Texture2D* raw_texture)
{
    LogFunc;

    if (!d3d_device_ || !raw_texture) {
        LogError << "handle is null" << VAR_VOIDP(d3d_device_) << VAR_VOIDP(raw_texture);
        return false;
    }

    raw_texture->GetDesc(&texture_desc_); // basic info

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

void DesktopDupScreencap::uninit()
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

std::optional<cv::Mat> DesktopDupScreencap::screencap_impl()
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
        LogError << "falied to init_texture";
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
