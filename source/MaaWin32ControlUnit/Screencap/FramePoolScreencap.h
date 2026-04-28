#pragma once

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

#include "SafeDXGI.hpp"

#include <vector>
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
    virtual ScreencapInfo last_screencap_info() const override { return last_screencap_info_; }

private:
    bool init();
    bool try_init_frame_pool(
        const winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice& d3d_device_interop,
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat pixel_format);
    bool init_texture(winrt::com_ptr<ID3D11Texture2D> raw_texture);
    bool init_hdr_gpu_pipeline();
    bool ensure_hdr_gpu_resources(const D3D11_TEXTURE2D_DESC& source_desc, const cv::Size& output_size);
    bool try_process_hdr_on_gpu(const winrt::com_ptr<ID3D11Texture2D>& texture, const cv::Rect& client_roi, cv::Mat& output);
    void reset_hdr_gpu_resources();
    void uninit();
    bool check_and_handle_size_changed();
    void try_disable_border();
    void try_disable_cursor();
    void try_include_secondary_windows();
    bool include_secondary_windows_enabled() const { return include_secondary_windows_enabled_; }

private:
    HWND hwnd_ = nullptr;

    winrt::com_ptr<ID3D11Device> d3d_device_ = nullptr;
    winrt::com_ptr<ID3D11DeviceContext> d3d_context_ = nullptr;
    winrt::com_ptr<IDXGISwapChain> dxgi_swap_chain_ = nullptr;
    winrt::com_ptr<ID3D11Texture2D> readable_texture_ = nullptr;
    D3D11_TEXTURE2D_DESC texture_desc_ = { };

    winrt::Windows::Graphics::Capture::GraphicsCaptureItem cap_item_ = nullptr;
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool cap_frame_pool_ = nullptr;
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession cap_session_ = nullptr;
    winrt::Windows::Graphics::DirectX::DirectXPixelFormat cap_pixel_format_ =
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized;
    HMONITOR last_monitor_ = nullptr;
    std::vector<HMONITOR> last_capture_monitors_;
    bool last_capture_prefers_hdr_ = true;

    // 存储上次的窗口大小，用于检测窗口大小变化
    std::pair<int, int> last_capture_size_ = { 0, 0 };
    cv::Mat cached_image_;
    bool include_secondary_windows_enabled_ = false;
    ScreencapInfo last_screencap_info_ {};

    bool hdr_gpu_pipeline_attempted_ = false;
    bool hdr_gpu_pipeline_available_ = false;
    D3D11_TEXTURE2D_DESC hdr_gpu_source_desc_ = { };
    cv::Size hdr_gpu_output_size_ {};
    cv::Size hdr_gpu_stats_size_ {};
    winrt::com_ptr<ID3D11VertexShader> hdr_vertex_shader_ = nullptr;
    winrt::com_ptr<ID3D11PixelShader> hdr_copy_pixel_shader_ = nullptr;
    winrt::com_ptr<ID3D11PixelShader> hdr_tonemap_pixel_shader_ = nullptr;
    winrt::com_ptr<ID3D11SamplerState> hdr_sampler_state_ = nullptr;
    winrt::com_ptr<ID3D11Buffer> hdr_constant_buffer_ = nullptr;
    winrt::com_ptr<ID3D11Texture2D> hdr_shader_input_texture_ = nullptr;
    winrt::com_ptr<ID3D11ShaderResourceView> hdr_shader_input_srv_ = nullptr;
    winrt::com_ptr<ID3D11Texture2D> hdr_stats_texture_ = nullptr;
    winrt::com_ptr<ID3D11RenderTargetView> hdr_stats_rtv_ = nullptr;
    winrt::com_ptr<ID3D11Texture2D> hdr_stats_staging_ = nullptr;
    winrt::com_ptr<ID3D11Texture2D> hdr_output_texture_ = nullptr;
    winrt::com_ptr<ID3D11RenderTargetView> hdr_output_rtv_ = nullptr;
    winrt::com_ptr<ID3D11Texture2D> hdr_output_staging_ = nullptr;
};

MAA_CTRL_UNIT_NS_END

#else

MAA_CTRL_UNIT_NS_BEGIN

#include "MaaUtils/SafeWindows.hpp"

class FramePoolScreencap : public ScreencapBase
{
public:
    FramePoolScreencap(HWND) { }

    virtual ~FramePoolScreencap() override = default;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override { return { }; }
};

MAA_CTRL_UNIT_NS_END

#endif
