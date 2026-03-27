#include "FramePoolScreencap.h"

#if MAA_FRAMEPOOL_SCREENCAP_AVAILABLE

#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Security.Authorization.AppCapabilityAccess.h>

#include <thread>

#include "HwndUtils.hpp"
#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "MaaUtils/Time.hpp"

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

    // 检查窗口大小是否变化，如果变化则重新创建 frame pool
    if (!check_and_handle_size_changed()) {
        LogError << "check_and_handle_size_changed failed";
        return std::nullopt;
    }

    winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame frame = nullptr;

    try {
        // 先清空 FramePool 中可能残留的旧帧
        while (auto old_frame = cap_frame_pool_.TryGetNextFrame()) {
            old_frame.Close();
        }

        // 等待新帧到来
        using namespace std::chrono_literals;
        auto start_time = std::chrono::steady_clock::now();
        while (duration_since(start_time) < 2000ms) {
            std::this_thread::sleep_for(2ms);
            frame = cap_frame_pool_.TryGetNextFrame();
            if (frame) {
                break;
            }
        }
    }
    catch (const winrt::hresult_error& e) {
        LogError << "Failed to get frame" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        uninit();
        return std::nullopt;
    }

    if (!frame) {
        if (cached_image_.empty()) {
            LogError << "Failed to get frame and no cached image available";
            return std::nullopt;
        }
        return cached_image_.clone();
    }

    auto surface = frame.Surface();
    if (!surface) {
        LogError << "frame.Surface() is null";
        return std::nullopt;
    }

    auto access = surface.try_as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    if (!access) {
        LogError << "Failed to get IDirect3DDxgiInterfaceAccess";
        return std::nullopt;
    }

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

    D3D11_MAPPED_SUBRESOURCE mapped { 0 };
    ret = d3d_context_->Map(readable_texture_.get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(ret)) {
        LogError << "Map failed" << VAR(ret);
        return std::nullopt;
    }
    OnScopeLeave([&]() { d3d_context_->Unmap(readable_texture_.get(), 0); });

    cv::Mat raw(texture_desc_.Height, texture_desc_.Width, CV_8UC4, mapped.pData, mapped.RowPitch);

    // 先按 alpha 通道裁剪掉四周 alpha != 255 的边框
    cv::Mat alpha_channel;
    cv::extractChannel(raw, alpha_channel, 3);

    cv::Mat alpha_bin;
    cv::threshold(alpha_channel, alpha_bin, UCHAR_MAX - 1, UCHAR_MAX, cv::THRESH_BINARY);

    cv::Rect alpha_roi = cv::boundingRect(alpha_bin);
    if (alpha_roi.empty()) {
        LogError << "No opaque pixels found";
        return std::nullopt;
    }
    cv::Mat trimmed = raw(alpha_roi);

    // 获取窗口客户区矩形（相对于窗口）
    RECT client_rect = { 0 };
    if (!GetClientRect(hwnd_, &client_rect)) {
        LogError << "GetClientRect failed";
        return std::nullopt;
    }

    // 将客户区左上角转换为屏幕坐标
    POINT client_top_left = { client_rect.left, client_rect.top };
    if (!ClientToScreen(hwnd_, &client_top_left)) {
        LogError << "ClientToScreen failed";
        return std::nullopt;
    }

    // 获取窗口矩形（屏幕坐标）
    RECT window_rect = { 0 };
    if (!GetWindowRect(hwnd_, &window_rect)) {
        LogError << "GetWindowRect failed";
        return std::nullopt;
    }

    // 计算边框位置，减去 alpha 裁剪的偏移
    int border_left = client_top_left.x - window_rect.left - alpha_roi.x;
    int border_top = client_top_left.y - window_rect.top - alpha_roi.y;

    // 获取客户区大小
    int client_width = client_rect.right - client_rect.left;
    int client_height = client_rect.bottom - client_rect.top;

    if (border_left < 0) {
        border_left = 0;
    }
    if (border_top < 0) {
        border_top = 0;
    }
    if (client_width > trimmed.cols) {
        client_width = trimmed.cols;
    }
    if (border_left + client_width > trimmed.cols) {
        border_left = trimmed.cols - client_width;
    }
    if (client_height > trimmed.rows) {
        client_height = trimmed.rows;
    }
    if (border_top + client_height > trimmed.rows) {
        border_top = trimmed.rows - client_height;
    }

    // 裁剪出客户区（去掉边框）
    cv::Rect client_roi(border_left, border_top, client_width, client_height);
    cv::Mat image = trimmed(client_roi);

    cv::Mat result = bgra_to_bgr(image);
    cached_image_ = result.clone();
    return result;
}

bool FramePoolScreencap::init()
{
    LogFunc;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    HRESULT ret = S_OK;

    DXGI_SWAP_CHAIN_DESC swap_chain_desc = { };
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

    try {
        auto activation_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
        auto interop_factory = activation_factory.try_as<IGraphicsCaptureItemInterop>();
        if (!interop_factory) {
            LogError << "Failed to get IGraphicsCaptureItemInterop";
            return false;
        }
        ret = interop_factory->CreateForWindow(
            hwnd_,
            winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
            winrt::put_abi(cap_item_));
    }
    catch (const winrt::hresult_error& e) {
        LogError << "Failed to create GraphicsCaptureItem" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        return false;
    }
    if (FAILED(ret)) {
        LogError << "CreateForWindow GraphicsCaptureItem failed" << VAR(ret);
        return false;
    }

    if (!cap_item_) {
        LogError << "cap_item_ is null after CreateForWindow";
        return false;
    }

    auto item_size = cap_item_.Size();
    if (item_size.Width <= 0 || item_size.Height <= 0) {
        LogError << "Invalid capture item size" << VAR(item_size.Width) << VAR(item_size.Height);
        return false;
    }

    if (!IsWindow(hwnd_) || !IsWindowVisible(hwnd_)) {
        LogError << "Window is no longer valid or visible";
        return false;
    }

    auto dxgi_device = d3d_device_.try_as<IDXGIDevice>();
    if (!dxgi_device) {
        LogError << "Failed to get IDXGIDevice";
        return false;
    }

    winrt::com_ptr<IInspectable> inspectable = nullptr;
    ret = CreateDirect3D11DeviceFromDXGIDevice(dxgi_device.get(), inspectable.put());
    if (FAILED(ret)) {
        LogError << "CreateDirect3D11DeviceFromDXGIDevice failed" << VAR(ret);
        return false;
    }

    auto d3d_device_interop = inspectable.try_as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
    if (!d3d_device_interop) {
        LogError << "Failed to get IDirect3DDevice";
        return false;
    }

    try {
        cap_frame_pool_ = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::Create(
            d3d_device_interop,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            1,
            cap_item_.Size());
    }
    catch (const winrt::hresult_error& e) {
        LogError << "Direct3D11CaptureFramePool::Create failed" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        return false;
    }

    if (!cap_frame_pool_) {
        LogError << "Direct3D11CaptureFramePool::Create returned null";
        return false;
    }

    try {
        cap_session_ = cap_frame_pool_.CreateCaptureSession(cap_item_);
    }
    catch (const winrt::hresult_error& e) {
        LogError << "CreateCaptureSession failed" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        return false;
    }
    if (!cap_session_) {
        LogError << "CreateCaptureSession returned null";
        return false;
    }

    // 尝试关闭截图时的黄色边框（Windows 11 及部分 Win10 版本支持）
    try_disable_border();

    // 尝试关闭截图时的鼠标指针（Windows 10 2004 及以上支持）
    try_disable_cursor();

    // 尝试包含从属窗口（弹窗、工具提示等）
    try_include_secondary_windows();

    try {
        cap_session_.StartCapture();
    }
    catch (const winrt::hresult_error& e) {
        LogError << "StartCapture failed" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        return false;
    }

    // 记录初始窗口大小
    if (cap_item_) {
        auto size = cap_item_.Size();
        last_capture_size_.first = size.Width;
        last_capture_size_.second = size.Height;
    }

    return true;
}

void FramePoolScreencap::uninit()
{
    if (cap_session_) {
        try {
            cap_session_.Close();
        }
        catch (const winrt::hresult_error& e) {
            LogWarn << "cap_session_.Close() failed" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        }
        cap_session_ = nullptr;
    }

    if (cap_frame_pool_) {
        try {
            cap_frame_pool_.Close();
        }
        catch (const winrt::hresult_error& e) {
            LogWarn << "cap_frame_pool_.Close() failed" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        }
        cap_frame_pool_ = nullptr;
    }

    readable_texture_ = nullptr;
    cap_item_ = nullptr;
    texture_desc_ = { 0 };
    last_capture_size_ = { };
}

bool FramePoolScreencap::check_and_handle_size_changed()
{
    if (!cap_item_) {
        return true;
    }

    if (!IsWindow(hwnd_)) {
        LogError << "Window is no longer valid";
        return false;
    }

    winrt::Windows::Graphics::SizeInt32 current_size { };
    try {
        current_size = cap_item_.Size();
    }
    catch (const winrt::hresult_error& e) {
        LogError << "Failed to get capture item size" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        uninit();
        return false;
    }
    // 如果窗口大小没有变化，直接返回
    if (current_size.Width == last_capture_size_.first && current_size.Height == last_capture_size_.second) {
        return true;
    }

    LogInfo << "Window size changed, recreating frame pool" << VAR(current_size.Width) << VAR(current_size.Height)
            << VAR(last_capture_size_.first) << VAR(last_capture_size_.second);

    // 完全重新初始化以适应新的窗口大小
    uninit();
    if (!init()) {
        LogError << "reinit failed after size change";
        return false;
    }

    return true;
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

void FramePoolScreencap::try_disable_border()
{
    LogFunc;

    using namespace winrt::Windows::Foundation::Metadata;
    using namespace winrt::Windows::Graphics::Capture;

    // GraphicsCaptureAccess 和 IsBorderRequired 在 UniversalApiContract v10.0 (Windows 10 2004) 引入
    if (!ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 10)) {
        LogInfo << "UniversalApiContract v10 not present, border toggle not supported";
        return;
    }

    if (!ApiInformation::IsTypePresent(L"Windows.Graphics.Capture.GraphicsCaptureAccess")) {
        LogInfo << "GraphicsCaptureAccess not present, border toggle not supported";
        return;
    }

    if (!ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IsBorderRequired")) {
        LogInfo << "IsBorderRequired property not supported on this system";
        return;
    }

    try {
        auto op = GraphicsCaptureAccess::RequestAccessAsync(GraphicsCaptureAccessKind::Borderless);
        auto status = op.wait_for(std::chrono::seconds(5));
        if (status != winrt::Windows::Foundation::AsyncStatus::Completed) {
            LogWarn << "RequestAccessAsync did not complete in time";
            return;
        }

        auto access_result = op.GetResults();
        if (access_result != winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapabilityAccessStatus::Allowed) {
            LogWarn << "Borderless capture access not granted:" << static_cast<int>(access_result);
            return;
        }

        cap_session_.IsBorderRequired(false);
        LogInfo << "Capture border disabled successfully";
    }
    catch (const winrt::hresult_error& e) {
        LogWarn << "Failed to disable capture border" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
    }
}

void FramePoolScreencap::try_disable_cursor()
{
    LogFunc;

    using namespace winrt::Windows::Foundation::Metadata;

    if (!ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IsCursorCaptureEnabled")) {
        LogInfo << "IsCursorCaptureEnabled property not supported on this system";
        return;
    }

    try {
        cap_session_.IsCursorCaptureEnabled(false);
        LogInfo << "Cursor capture disabled successfully";
    }
    catch (const winrt::hresult_error& e) {
        LogWarn << "Failed to disable cursor capture" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
    }
}

void FramePoolScreencap::try_include_secondary_windows()
{
    LogFunc;

    using namespace winrt::Windows::Foundation::Metadata;

    if (!ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IncludeSecondaryWindows")) {
        LogInfo << "IncludeSecondaryWindows property not supported on this system";
        return;
    }

    try {
        cap_session_.IncludeSecondaryWindows(true);
        LogInfo << "Secondary windows capture enabled successfully";
    }
    catch (const winrt::hresult_error& e) {
        LogWarn << "Failed to enable secondary windows capture" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
    }
}

MAA_CTRL_UNIT_NS_END

#endif
