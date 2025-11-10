#pragma once

#include "Base/UnitBase.h"
#include "SafeDXGI.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class DesktopDupScreencap : public ScreencapBase
{
public:
    explicit DesktopDupScreencap(HWND hwnd = nullptr)
        : hwnd_(hwnd)
    {
    }

    virtual ~DesktopDupScreencap() override;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override;

protected:
    bool init();
    bool init_texture(ID3D11Texture2D* raw_texture);
    void uninit();
    std::optional<cv::Mat> screencap_impl();

    ID3D11Device* d3d_device_ = nullptr;
    ID3D11DeviceContext* d3d_context_ = nullptr;
    IDXGIFactory* dxgi_factory_ = nullptr;
    IDXGIAdapter* dxgi_adapter_ = nullptr;
    IDXGIOutput1* dxgi_output_ = nullptr;
    IDXGIOutputDuplication* dxgi_dup_ = nullptr;

    ID3D11Texture2D* readable_texture_ = nullptr;
    D3D11_TEXTURE2D_DESC texture_desc_ { 0 };

    HWND hwnd_ = nullptr;
    HMONITOR current_monitor_ = nullptr;
    bool output_just_initialized_ = false;

private:
    bool init_d3d_device();
    bool init_dxgi_factory();
    bool find_output_by_monitor(HMONITOR monitor);
    bool init_primary_output();
    bool init_output_duplication();
    bool ensure_output_for_monitor();
};

MAA_CTRL_UNIT_NS_END
