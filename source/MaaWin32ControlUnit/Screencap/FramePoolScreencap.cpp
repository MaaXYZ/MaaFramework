#include "FramePoolScreencap.h"

#if MAA_FRAMEPOOL_SCREENCAP_AVAILABLE

#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.h>

#include "HwndUtils.hpp"
#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_CTRL_UNIT_NS_BEGIN

FramePoolScreencap::~FramePoolScreencap()
{
    uninit();
}

std::optional<cv::Mat> FramePoolScreencap::screencap()
{
    if (!cap_frame_pool_) {
        if (!init()) {
            LogError << "init failed";
            uninit();
            return std::nullopt;
        }
    }

    std::unique_lock lock(frame_mutex_);

    constexpr size_t kTimeoutSec = 20;
    if (!latest_frame_ && frame_cv_.wait_for(lock, std::chrono::seconds(kTimeoutSec)) == std::cv_status::timeout) {
        LogError << "wait for frame timeout";
        return std::nullopt;
    }

    auto access = latest_frame_.Surface().as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();

    winrt::com_ptr<ID3D11Texture2D> texture = nullptr;
    HRESULT ret = access->GetInterface(winrt::guid_of<ID3D11Texture2D>(), texture.put_void());
    if (FAILED(ret)) {
        LogError << "GetInterface ID3D11Texture2D failed" << VAR(ret);
        return std::nullopt;
    }

    if (!readable_texture_ && !init_texture(texture)) {
        LogError << "falied to init_texture";
        return std::nullopt;
    }
    d3d_context_->CopyResource(readable_texture_.get(), texture.get());

    lock.unlock();

    D3D11_MAPPED_SUBRESOURCE mapped { 0 };
    ret = d3d_context_->Map(readable_texture_.get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(ret)) {
        LogError << "Map failed" << VAR(ret);
        return std::nullopt;
    }
    OnScopeLeave([&]() { d3d_context_->Unmap(readable_texture_.get(), 0); });

    cv::Mat raw(texture_desc_.Height, texture_desc_.Width, CV_8UC4, mapped.pData, mapped.RowPitch);

    std::vector<cv::Mat> channels;
    cv::split(raw, channels);
    cv::Mat alpha_bin;
    cv::threshold(channels.back(), alpha_bin, UCHAR_MAX - 1, UCHAR_MAX, cv::THRESH_BINARY);

    cv::Rect boundary = cv::boundingRect(alpha_bin);
    cv::Mat image = raw(boundary);

    return bgra_to_bgr(image);
}

void FramePoolScreencap::frame_handler(
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const&,
    winrt::Windows::Foundation::IInspectable const&)
{
    std::unique_lock lock(frame_mutex_);
    latest_frame_ = cap_frame_pool_.TryGetNextFrame();
    frame_cv_.notify_one();
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
    swap_chain_desc.BufferCount = 1;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.OutputWindow = hwnd_;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.Windowed = TRUE;

    ret = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        dxgi_swap_chain_.put(),
        d3d_device_.put(),
        nullptr,
        d3d_context_.put());

    if (FAILED(ret)) {
        LogError << "D3D11CreateDeviceAndSwapChain failed" << VAR(ret);
        return false;
    }

    auto activation_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
    auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
    ret = interop_factory->CreateForWindow(
        hwnd_,
        winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
        winrt::put_abi(cap_item_));
    if (FAILED(ret)) {
        LogError << "CreateForWindow GraphicsCaptureItem failed" << VAR(ret);
        return false;
    }

    winrt::com_ptr<IDXGIDevice> dxgi_device = d3d_device_.as<IDXGIDevice>();

    winrt::com_ptr<IInspectable> inspectable = nullptr;
    ret = CreateDirect3D11DeviceFromDXGIDevice(dxgi_device.get(), inspectable.put());
    if (FAILED(ret)) {
        LogError << "CreateDirect3D11DeviceFromDXGIDevice failed" << VAR(ret);
        return false;
    }

    cap_frame_pool_ = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(
        inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>(),
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
        2,
        cap_item_.Size());

    if (!cap_frame_pool_) {
        LogError << "Direct3D11CaptureFramePool::Create failed";
        return false;
    }

    frame_arrived_token_ = cap_frame_pool_.FrameArrived({ this, &FramePoolScreencap::frame_handler });

    cap_session_ = cap_frame_pool_.CreateCaptureSession(cap_item_);
    if (!cap_session_) {
        LogError << "CreateCaptureSession failed";
        return false;
    }

    cap_session_.StartCapture();

    return true;
}

void FramePoolScreencap::uninit()
{
    texture_desc_ = { 0 };
}

bool FramePoolScreencap::init_texture(winrt::com_ptr<ID3D11Texture2D> raw_texture)
{
    LogFunc;

    if (!d3d_device_ || !raw_texture) {
        LogError << "handle is null";
        return false;
    }

    raw_texture->GetDesc(&texture_desc_);

    texture_desc_.BindFlags = 0;
    texture_desc_.MiscFlags = 0;
    texture_desc_.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    texture_desc_.Usage = D3D11_USAGE_STAGING;

    HRESULT ret = d3d_device_->CreateTexture2D(&texture_desc_, NULL, readable_texture_.put());
    if (FAILED(ret)) {
        LogError << "CreateTexture2D failed" << VAR(ret);
        return false;
    }

    return true;
}

MAA_CTRL_UNIT_NS_END

#endif
