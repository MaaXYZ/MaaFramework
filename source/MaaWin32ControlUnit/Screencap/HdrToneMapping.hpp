#pragma once

#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

// 检测纹理格式是否为 HDR（浮点格式）
inline bool is_hdr_format(DXGI_FORMAT format)
{
    switch (format) {
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
        return true;
    default:
        return false;
    }
}

// 检测当前显示器是否开启了 HDR（通过 DXGI）
inline bool is_hdr_enabled_on_output(IDXGIOutput* output)
{
    if (!output) {
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
    HRESULT hr = output->QueryInterface(IID_PPV_ARGS(&output6));
    if (FAILED(hr)) {
        return false; // 老版本 Windows，不支持 HDR
    }

    DXGI_OUTPUT_DESC1 desc1;
    hr = output6->GetDesc1(&desc1);
    if (FAILED(hr)) {
        return false;
    }

    // 检查色彩空间是否为 HDR（BT.2020 + PQ/HLG）
    return desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 ||
           desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020;
}

// HDR Tone Mapping 处理器，使用 Direct2D HDR Tone Map Effect
class HdrToneMapper
{
public:
    HdrToneMapper() = default;
    ~HdrToneMapper() { uninit(); }

    // 禁用拷贝
    HdrToneMapper(const HdrToneMapper&) = delete;
    HdrToneMapper& operator=(const HdrToneMapper&) = delete;

    // 对 HDR 纹理进行 Tone Mapping，返回 SDR 的 cv::Mat (BGRA 8bit)
    // 如果纹理不是 HDR 格式，直接返回 nullopt（表示不需要处理）
    std::optional<cv::Mat> tone_map(ID3D11Device* d3d_device, ID3D11DeviceContext* d3d_context, ID3D11Texture2D* texture)
    {
        if (!d3d_device || !d3d_context || !texture) {
            LogError << "Invalid parameters";
            return std::nullopt;
        }

        D3D11_TEXTURE2D_DESC desc;
        texture->GetDesc(&desc);

        if (!is_hdr_format(desc.Format)) {
            return std::nullopt; // 非 HDR 格式，不需要处理
        }

        LogDebug << "HDR texture detected, applying tone mapping" << VAR(desc.Format)
                 << VAR(desc.Width) << VAR(desc.Height);

        if (!ensure_initialized(d3d_device)) {
            LogError << "Failed to initialize D2D resources";
            return std::nullopt;
        }

        return apply_tone_mapping(d3d_device, d3d_context, texture, desc);
    }

private:
    bool ensure_initialized(ID3D11Device* d3d_device)
    {
        if (d2d_context_) {
            return true;
        }

        LogFunc;

        HRESULT hr = S_OK;

        // 创建 D2D1 Factory
        D2D1_FACTORY_OPTIONS options = {};
#ifdef _DEBUG
        options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, d2d_factory_.GetAddressOf());
        if (FAILED(hr)) {
            LogError << "D2D1CreateFactory failed" << VAR(hr);
            return false;
        }

        // 获取 DXGI Device
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
        hr = d3d_device->QueryInterface(IID_PPV_ARGS(&dxgi_device));
        if (FAILED(hr)) {
            LogError << "QueryInterface IDXGIDevice failed" << VAR(hr);
            return false;
        }

        // 创建 D2D1 Device
        hr = d2d_factory_->CreateDevice(dxgi_device.Get(), d2d_device_.GetAddressOf());
        if (FAILED(hr)) {
            LogError << "CreateDevice failed" << VAR(hr);
            return false;
        }

        // 创建 D2D1 DeviceContext
        hr = d2d_device_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2d_context_.GetAddressOf());
        if (FAILED(hr)) {
            LogError << "CreateDeviceContext failed" << VAR(hr);
            return false;
        }

        return true;
    }

    std::optional<cv::Mat> apply_tone_mapping(
        ID3D11Device* d3d_device,
        ID3D11DeviceContext* d3d_context,
        ID3D11Texture2D* texture,
        const D3D11_TEXTURE2D_DESC& desc)
    {
        HRESULT hr = S_OK;

        // Desktop Duplication 返回的纹理可能不支持直接被 D2D 访问，
        // 需要先复制到一个支持 D2D 共享的纹理
        D3D11_TEXTURE2D_DESC shared_desc = desc;
        shared_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        shared_desc.MiscFlags = 0;
        shared_desc.CPUAccessFlags = 0;
        shared_desc.Usage = D3D11_USAGE_DEFAULT;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> shared_texture;
        hr = d3d_device->CreateTexture2D(&shared_desc, nullptr, shared_texture.GetAddressOf());
        if (FAILED(hr)) {
            LogError << "CreateTexture2D for shared texture failed" << VAR(hr);
            return std::nullopt;
        }

        d3d_context->CopyResource(shared_texture.Get(), texture);

        // 获取 DXGI Surface
        Microsoft::WRL::ComPtr<IDXGISurface> dxgi_surface;
        hr = shared_texture->QueryInterface(IID_PPV_ARGS(&dxgi_surface));
        if (FAILED(hr)) {
            LogError << "QueryInterface IDXGISurface failed" << VAR(hr);
            return std::nullopt;
        }

        // 创建输入位图（HDR）
        D2D1_BITMAP_PROPERTIES1 input_props = {};
        input_props.pixelFormat.format = desc.Format;
        input_props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        input_props.bitmapOptions = D2D1_BITMAP_OPTIONS_NONE;

        Microsoft::WRL::ComPtr<ID2D1Bitmap1> input_bitmap;
        hr = d2d_context_->CreateBitmapFromDxgiSurface(dxgi_surface.Get(), &input_props, input_bitmap.GetAddressOf());
        if (FAILED(hr)) {
            LogError << "CreateBitmapFromDxgiSurface failed" << VAR(hr);
            return std::nullopt;
        }

        // 创建输出位图（SDR, BGRA 8bit）用于渲染目标
        D2D1_BITMAP_PROPERTIES1 output_props = {};
        output_props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        output_props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        output_props.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

        Microsoft::WRL::ComPtr<ID2D1Bitmap1> output_bitmap;
        hr = d2d_context_->CreateBitmap(D2D1::SizeU(desc.Width, desc.Height), nullptr, 0, &output_props, output_bitmap.GetAddressOf());
        if (FAILED(hr)) {
            LogError << "CreateBitmap for output failed" << VAR(hr);
            return std::nullopt;
        }

        // 创建可读取的位图（用于复制结果）
        D2D1_BITMAP_PROPERTIES1 readable_props = {};
        readable_props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        readable_props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        readable_props.bitmapOptions = D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

        Microsoft::WRL::ComPtr<ID2D1Bitmap1> readable_bitmap;
        hr = d2d_context_->CreateBitmap(D2D1::SizeU(desc.Width, desc.Height), nullptr, 0, &readable_props, readable_bitmap.GetAddressOf());
        if (FAILED(hr)) {
            LogError << "CreateBitmap for readable failed" << VAR(hr);
            return std::nullopt;
        }

        // 创建 HDR Tone Map Effect
        Microsoft::WRL::ComPtr<ID2D1Effect> tone_map_effect;
        hr = d2d_context_->CreateEffect(CLSID_D2D1HdrToneMap, tone_map_effect.GetAddressOf());
        if (FAILED(hr)) {
            LogError << "CreateEffect CLSID_D2D1HdrToneMap failed" << VAR(hr)
                     << "(requires Windows 10 1809 or later)";
            return std::nullopt;
        }

        // 设置 Tone Map 参数
        // 输入最大亮度：典型 HDR 内容约 1000-4000 nits，使用较高值以避免过度压缩
        constexpr float input_max_luminance = 1000.0f;  // nits
        constexpr float output_max_luminance = 80.0f;   // SDR 标准亮度
        tone_map_effect->SetValue(D2D1_HDRTONEMAP_PROP_INPUT_MAX_LUMINANCE, input_max_luminance);
        tone_map_effect->SetValue(D2D1_HDRTONEMAP_PROP_OUTPUT_MAX_LUMINANCE, output_max_luminance);
        tone_map_effect->SetValue(D2D1_HDRTONEMAP_PROP_DISPLAY_MODE, D2D1_HDRTONEMAP_DISPLAY_MODE_SDR);

        // 设置输入
        tone_map_effect->SetInput(0, input_bitmap.Get());

        // 渲染
        d2d_context_->SetTarget(output_bitmap.Get());
        d2d_context_->BeginDraw();
        d2d_context_->Clear(D2D1::ColorF(0, 0, 0, 0));
        d2d_context_->DrawImage(tone_map_effect.Get());
        hr = d2d_context_->EndDraw();

        if (FAILED(hr)) {
            LogError << "EndDraw failed" << VAR(hr);
            return std::nullopt;
        }

        // 复制到可读位图
        D2D1_POINT_2U dest_point = { 0, 0 };
        D2D1_RECT_U src_rect = { 0, 0, desc.Width, desc.Height };
        hr = readable_bitmap->CopyFromBitmap(&dest_point, output_bitmap.Get(), &src_rect);
        if (FAILED(hr)) {
            LogError << "CopyFromBitmap failed" << VAR(hr);
            return std::nullopt;
        }

        // Map 并读取像素数据
        D2D1_MAPPED_RECT mapped;
        hr = readable_bitmap->Map(D2D1_MAP_OPTIONS_READ, &mapped);
        if (FAILED(hr)) {
            LogError << "Map failed" << VAR(hr);
            return std::nullopt;
        }

        // 复制到 cv::Mat
        cv::Mat result(desc.Height, desc.Width, CV_8UC4);
        for (UINT y = 0; y < desc.Height; ++y) {
            memcpy(result.ptr(y), mapped.bits + y * mapped.pitch, desc.Width * 4);
        }

        readable_bitmap->Unmap();

        LogDebug << "HDR tone mapping completed";
        return result;
    }

    void uninit()
    {
        d2d_context_.Reset();
        d2d_device_.Reset();
        d2d_factory_.Reset();
    }

private:
    Microsoft::WRL::ComPtr<ID2D1Factory1> d2d_factory_;
    Microsoft::WRL::ComPtr<ID2D1Device> d2d_device_;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2d_context_;
};

MAA_CTRL_UNIT_NS_END
