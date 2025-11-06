#pragma once

#include "Base/UnitBase.h"
#include "SafeDXGI.hpp"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class DesktopDupWindowScreencap : public ScreencapBase
{
public:
    explicit DesktopDupWindowScreencap(HWND hwnd)
        : hwnd_(hwnd)
    {
    }

    virtual ~DesktopDupWindowScreencap() override;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override;

private:
    bool init();
    bool init_texture(ID3D11Texture2D* raw_texture);
    void uninit();
    std::optional<cv::Mat> screencap_impl();
    RECT get_window_client_rect_screen() const;

    HWND hwnd_ = nullptr;

    ID3D11Device* d3d_device_ = nullptr;
    ID3D11DeviceContext* d3d_context_ = nullptr;
    IDXGIFactory* dxgi_factory_ = nullptr;
    IDXGIAdapter* dxgi_adapter_ = nullptr;
    IDXGIOutput1* dxgi_output_ = nullptr;
    IDXGIOutputDuplication* dxgi_dup_ = nullptr;

    ID3D11Texture2D* readable_texture_ = nullptr;
    D3D11_TEXTURE2D_DESC texture_desc_ { 0 };
};

MAA_CTRL_UNIT_NS_END

