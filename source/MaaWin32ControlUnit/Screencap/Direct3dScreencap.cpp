#include "Direct3dScreencap.h"

#include <d3d11.h>
#include <dxgi1_6.h>

#include "Utils/ImageIo.h"
#include "Utils/Logger.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

MAA_CTRL_UNIT_NS_BEGIN

std::optional<cv::Mat> Direct3dScreencap::screencap()
{
    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;
    OnScopeLeave([&]() {
        if (d3dContext) {
            d3dContext->Release();
        }
        if (d3dDevice) {
            d3dDevice->Release();
        }
    });

    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                   &d3dDevice, nullptr, &d3dContext);
    if (FAILED(hr)) {
        LogError << "D3D11CreateDevice failed" << VAR(hr);
        return std::nullopt;
    }

    IDXGIFactory* dxgiFactory = nullptr;
    OnScopeLeave([&]() {
        if (dxgiFactory) {
            dxgiFactory->Release();
        }
    });

    hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
    if (FAILED(hr)) {
        LogError << "CreateDXGIFactory failed" << VAR(hr);
        return std::nullopt;
    }

    IDXGIAdapter* dxgiAdapter = nullptr;
    OnScopeLeave([&]() {
        if (dxgiAdapter) {
            dxgiAdapter->Release();
        }
    });

    hr = dxgiFactory->EnumAdapters(0, &dxgiAdapter);
    if (FAILED(hr)) {
        LogError << "EnumAdapters failed" << VAR(hr);
        return std::nullopt;
    }

    IDXGIOutput1* dxgiOutput = nullptr;
    OnScopeLeave([&]() {
        if (dxgiOutput) {
            dxgiOutput->Release();
        }
    });
    hr = dxgiAdapter->EnumOutputs(0, (IDXGIOutput**)&dxgiOutput); // Cast to IDXGIOutput1
    if (FAILED(hr)) {
        LogError << "EnumOutputs failed" << VAR(hr);
        return std::nullopt;
    }

    IDXGIOutputDuplication* desktopDuplication = nullptr;
    OnScopeLeave([&]() {
        if (desktopDuplication) {
            desktopDuplication->Release();
        }
    });

    hr = dxgiOutput->DuplicateOutput(d3dDevice, &desktopDuplication);
    if (FAILED(hr)) {
        LogError << "DuplicateOutput failed" << VAR(hr);
        return std::nullopt;
    }

    IDXGIResource* desktopResource = nullptr;
    OnScopeLeave([&]() {
        if (desktopResource) {
            desktopResource->Release();
        }
    });
    DXGI_OUTDUPL_FRAME_INFO frameInfo { 0 };

    hr = desktopDuplication->AcquireNextFrame(INFINITE, &frameInfo, &desktopResource);
    if (FAILED(hr)) {
        LogError << "AcquireNextFrame failed" << VAR(hr);
        return std::nullopt;
    }

    ID3D11Texture2D* desktopTexture = nullptr;
    OnScopeLeave([&]() {
        if (desktopTexture) {
            desktopTexture->Release();
        }
    });

    hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&desktopTexture);
    if (FAILED(hr)) {
        LogError << "QueryInterface failed" << VAR(hr);
        return std::nullopt;
    }

    D3D11_TEXTURE2D_DESC desc { 0 };
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Width = 2560;
    desc.Height = 1440;
    desc.ArraySize = 1;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.MipLevels = 1;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;

    ID3D11Texture2D* currTexture = nullptr;
    OnScopeLeave([&]() {
        if (currTexture) {
            currTexture->Release();
        }
    });
    hr = d3dDevice->CreateTexture2D(&desc, NULL, &currTexture);
    if (FAILED(hr)) {
        LogError << "CreateTexture2D failed" << VAR(hr);
        return std::nullopt;
    }

    d3dContext->CopyResource(currTexture, desktopTexture);

    D3D11_MAPPED_SUBRESOURCE mapped { 0 };
    hr = d3dContext->Map(currTexture, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        LogError << "Map failed" << VAR(hr);
        return std::nullopt;
    }
    cv::Mat screenshot(desc.Height, desc.Width, CV_8UC4, mapped.pData, mapped.RowPitch);

    d3dContext->Unmap(desktopTexture, 0);

    return screenshot;
}

MAA_CTRL_UNIT_NS_END
