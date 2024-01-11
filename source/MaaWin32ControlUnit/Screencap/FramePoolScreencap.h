#pragma once

#include "ControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "SafeDXGI.hpp"

#include <winrt/Windows.Graphics.Capture.h>

MAA_CTRL_UNIT_NS_BEGIN

// Not work
class FramePoolScreencap : public ScreencapBase
{
public:
    FramePoolScreencap(HWND hwnd) : hwnd_(hwnd) {}
    virtual ~FramePoolScreencap() override;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override;

private:
    bool init();
    void uninit();

private:
    HWND hwnd_ = nullptr;

    winrt::com_ptr<ID3D11Device> d3d_device_ = nullptr;
    winrt::com_ptr<ID3D11DeviceContext> d3d_context_ = nullptr;
    winrt::com_ptr<IDXGISwapChain> dxgi_swap_chain_ = nullptr;
    winrt::com_ptr<ID3D11Texture2D> readable_texture_ = nullptr;
    D3D11_TEXTURE2D_DESC texture_desc_ = {};

    winrt::Windows::Graphics::Capture::GraphicsCaptureItem cap_item_ = nullptr;
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool cap_frame_pool_ = nullptr;
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession cap_session_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
