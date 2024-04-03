#pragma once

#include "Base/UnitBase.h"
#include "SafeDXGI.hpp"

MAA_CTRL_UNIT_NS_BEGIN

// Not work
class BackBufferScreencap : public ScreencapBase
{
public:
    BackBufferScreencap(HWND hwnd)
        : hwnd_(hwnd)
    {
    }

    virtual ~BackBufferScreencap() override;

public: // from ScreencapBase
    // Not work
    virtual std::optional<cv::Mat> screencap() override;

private:
    bool init();
    bool init_texture(ID3D11Texture2D* raw_texture);
    void uninit();

private:
    HWND hwnd_ = nullptr;

    ID3D11Device* d3d_device_ = nullptr;
    ID3D11DeviceContext* d3d_context_ = nullptr;
    IDXGISwapChain* dxgi_swap_chain_ = nullptr;
    ID3D11Texture2D* readable_texture_ = nullptr;
    D3D11_TEXTURE2D_DESC texture_desc_ = {};
};

MAA_CTRL_UNIT_NS_END
