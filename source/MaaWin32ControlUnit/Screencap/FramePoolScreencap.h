#pragma once

#include <condition_variable>
#include <mutex>

#include <sdkddkver.h>

#ifndef MAA_FRAMEPOOL_SCREENCAP_AVAILABLE
#if WDK_NTDDI_VERSION >= 0x0A00000B // NTDDI_WIN10_CO
#define MAA_FRAMEPOOL_SCREENCAP_AVAILABLE 1
#else                               // else of WDK_NTDDI_VERSION
#define MAA_FRAMEPOOL_SCREENCAP_AVAILABLE 0
#pragma message("Win32 FramePool needs Windows 10 SDK 10.0.22000.0 or higher")
#endif // end of WDK_NTDDI_VERSION
#endif // end of MAA_FRAMEPOOL_SCREENCAP_AVAILABLE

#include "Base/UnitBase.h"
#include "Common/Conf.h"

#if MAA_FRAMEPOOL_SCREENCAP_AVAILABLE

#include "HdrToneMapping.hpp"
#include "SafeDXGI.hpp"

#include <winrt/Windows.Graphics.Capture.h>

MAA_CTRL_UNIT_NS_BEGIN

class FramePoolScreencap : public ScreencapBase
{
public:
    FramePoolScreencap(HWND hwnd)
        : hwnd_(hwnd)
    {
    }

    virtual ~FramePoolScreencap() override;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override;

public:
    void frame_handler(
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
        winrt::Windows::Foundation::IInspectable const& args);

private:
    bool init();
    bool init_texture(winrt::com_ptr<ID3D11Texture2D> raw_texture);
    void uninit();
    bool check_and_handle_size_changed();
    void try_disable_border();
    std::optional<cv::Mat> crop_to_client_area(const cv::Mat& raw);

private:
    HWND hwnd_ = nullptr;

    winrt::com_ptr<ID3D11Device> d3d_device_ = nullptr;
    winrt::com_ptr<ID3D11DeviceContext> d3d_context_ = nullptr;
    winrt::com_ptr<IDXGISwapChain> dxgi_swap_chain_ = nullptr;
    winrt::com_ptr<ID3D11Texture2D> readable_texture_ = nullptr;
    D3D11_TEXTURE2D_DESC texture_desc_ = {};

    std::mutex frame_mutex_;
    std::condition_variable frame_cv_;
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame latest_frame_ = nullptr;
    winrt::event_token frame_arrived_token_;

    winrt::Windows::Graphics::Capture::GraphicsCaptureItem cap_item_ = nullptr;
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool cap_frame_pool_ = nullptr;
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession cap_session_ = nullptr;

    // 存储上次的窗口大小，用于检测窗口大小变化
    std::pair<int, int> last_capture_size_ = { 0, 0 };

    HdrToneMapper hdr_tone_mapper_;
};

MAA_CTRL_UNIT_NS_END

#else

MAA_CTRL_UNIT_NS_BEGIN

#include "MaaUtils/SafeWindows.hpp"

class FramePoolScreencap : public ScreencapBase
{
public:
    FramePoolScreencap(HWND) {}

    virtual ~FramePoolScreencap() override = default;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override { return {}; }
};

MAA_CTRL_UNIT_NS_END

#endif
