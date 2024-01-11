#include "FramePoolScreencap.h"

#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>

#include "HwndUtils.hpp"
#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

FramePoolScreencap::~FramePoolScreencap()
{
    uninit();
}

std::optional<cv::Mat> FramePoolScreencap::screencap()
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return std::nullopt;
    }
    if (!cap_frame_pool_) {
        if (!init()) {
            LogError << "init failed";
            uninit();
            return std::nullopt;
        }
    }

    auto frame = cap_frame_pool_.TryGetNextFrame();
    if (frame == nullptr) {
        LogError << "TryGetNextFrame failed";
        return std::nullopt;
    }

    using namespace winrt::Windows::Graphics::DirectX;

    auto access = frame.Surface().as<Direct3D11::IDirect3DDxgiInterfaceAccess>();

    winrt::com_ptr<ID3D11Texture2D> texture = nullptr;
    HRESULT ret = access->GetInterface(winrt::guid_of<ID3D11Texture2D>(), texture.put_void());
    if (FAILED(ret)) {
        LogError << "GetInterface ID3D11Texture2D failed" << VAR(ret);
        return std::nullopt;
    }

    d3d_context_->CopyResource(readable_texture_.get(), texture.get());

    D3D11_MAPPED_SUBRESOURCE mapped { 0 };
    ret = d3d_context_->Map(readable_texture_.get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(ret)) {
        LogError << "Map failed" << VAR(ret);
        return std::nullopt;
    }
    OnScopeLeave([&]() { d3d_context_->Unmap(readable_texture_.get(), 0); });

    cv::Mat mat(texture_desc_.Height, texture_desc_.Width, CV_8UC4, mapped.pData, mapped.RowPitch);
    return mat.clone();
}

bool FramePoolScreencap::init()
{

    LogFunc;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    HRESULT ret = S_OK;

    DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.OutputWindow = hwnd_;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.Windowed = TRUE;

    ret = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                        &swap_chain_desc, dxgi_swap_chain_.put(), d3d_device_.put(), nullptr,
                                        d3d_context_.put());

    if (FAILED(ret)) {
        LogError << "D3D11CreateDeviceAndSwapChain failed" << VAR(ret);
        return false;
    }

    using namespace winrt::Windows::Graphics::Capture;
    using namespace winrt::Windows::Graphics::DirectX;

    auto activation_factory = winrt::get_activation_factory<GraphicsCaptureItem>();
    auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
    ret = interop_factory->CreateForWindow(
        hwnd_, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(cap_item_));
    if (FAILED(ret)) {
        LogError << "CreateForWindow GraphicsCaptureItem failed" << VAR(ret);
        return false;
    }

    texture_desc_ = D3D11_TEXTURE2D_DESC {
        .Width = static_cast<UINT>(cap_item_.Size().Width),
        .Height = static_cast<UINT>(cap_item_.Size().Height),
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
        .SampleDesc = { 1, 0 },
        .Usage = D3D11_USAGE_STAGING,
        .BindFlags = 0,
        .CPUAccessFlags = D3D11_CPU_ACCESS_READ,
        .MiscFlags = 0,
    };

    ret = d3d_device_->CreateTexture2D(&texture_desc_, NULL, readable_texture_.put());
    if (FAILED(ret)) {
        LogError << "CreateTexture2D failed" << VAR(ret);
        return false;
    }

    cap_frame_pool_ = Direct3D11CaptureFramePool::Create(
        d3d_device_.as<Direct3D11::IDirect3DDevice>(), DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, cap_item_.Size());

    if (!cap_frame_pool_) {
        LogError << "Direct3D11CaptureFramePool::Create failed";
        return false;
    }

    cap_session_ = cap_frame_pool_.CreateCaptureSession(cap_item_);
    if (!cap_session_) {
        LogError << "CreateCaptureSession failed";
        return false;
    }

    cap_session_.StartCapture();

    return true;
}

void FramePoolScreencap::uninit() {}

MAA_CTRL_UNIT_NS_END
