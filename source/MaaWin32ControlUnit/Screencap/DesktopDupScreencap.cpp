#include "DesktopDupScreencap.h"

#include "HwndUtils.hpp"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

DesktopDupScreencap::~DesktopDupScreencap()
{
    uninit();
}

std::optional<cv::Mat> DesktopDupScreencap::screencap()
{
    // 初始化 D3D 设备和 DXGI 工厂（只需要初始化一次）
    if (!d3d_device_) {
        if (!init()) {
            LogError << "failed to init_d3d_device";
            uninit();
            return std::nullopt;
        }
    }

    // 确保输出匹配当前窗口所在的显示器（每次截图时检查，支持窗口移动）
    if (!ensure_output_for_monitor()) {
        LogError << "failed to ensure_output_for_monitor";
        return std::nullopt;
    }

    // 如果输出刚初始化，前几张图片可能是空的，跳过
    if (output_just_initialized_) {
        for (int i = 0; i < 3; ++i) {
            auto opt = screencap_impl();
            if (opt) {
                const auto& br = *(opt->end<cv::Vec4b>() - 1);
                if (br[3] == 255) { // only check alpha
                    output_just_initialized_ = false;
                    break;
                }
            }
            LogWarn << "blank image, continue";
        }
    }

    auto img = screencap_impl();
    if (!img) {
        return std::nullopt;
    }
    return bgra_to_bgr(*img);
}

bool DesktopDupScreencap::init()
{
    LogFunc;

    if (!init_d3d_device()) {
        return false;
    }

    if (!init_dxgi_factory()) {
        return false;
    }

    return true;
}

bool DesktopDupScreencap::ensure_output_for_monitor()
{
    // 获取目标显示器
    HMONITOR target_monitor = nullptr;
    if (hwnd_) {
        target_monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
        if (!target_monitor) {
            LogWarn << "MonitorFromWindow failed, falling back to primary monitor";
        }
    }

    // 如果显示器没有改变，且输出已初始化，则不需要重新初始化
    if (target_monitor == current_monitor_ && dxgi_dup_) {
        return true;
    }

    // 显示器改变了或首次初始化，需要重新设置输出
    // 先释放旧的输出和纹理（因为分辨率可能改变了）
    if (dxgi_dup_) {
        dxgi_dup_->Release();
        dxgi_dup_ = nullptr;
    }
    if (readable_texture_) {
        readable_texture_->Release();
        readable_texture_ = nullptr;
    }
    if (dxgi_output_) {
        dxgi_output_->Release();
        dxgi_output_ = nullptr;
    }
    if (dxgi_adapter_) {
        dxgi_adapter_->Release();
        dxgi_adapter_ = nullptr;
    }

    // 尝试根据显示器查找输出，如果失败则使用主显示器
    bool found_output = false;
    if (target_monitor) {
        found_output = find_output_by_monitor(target_monitor);
    }

    if (!found_output) {
        if (!init_primary_output()) {
            return false;
        }
        current_monitor_ = nullptr; // 使用主显示器
    }
    else {
        current_monitor_ = target_monitor;
    }

    if (!init_output_duplication()) {
        return false;
    }

    // 标记输出刚初始化，需要跳过前几张空白图片
    output_just_initialized_ = true;

    return true;
}

bool DesktopDupScreencap::init_d3d_device()
{
    HRESULT ret = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &d3d_device_,
        nullptr,
        &d3d_context_);
    if (FAILED(ret)) {
        LogError << "D3D11CreateDevice failed" << VAR(ret);
        return false;
    }
    return true;
}

bool DesktopDupScreencap::init_dxgi_factory()
{
    HRESULT ret = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgi_factory_));
    if (FAILED(ret)) {
        LogError << "CreateDXGIFactory failed" << VAR(ret);
        return false;
    }
    return true;
}

bool DesktopDupScreencap::find_output_by_monitor(HMONITOR monitor)
{
    HRESULT ret = S_OK;

    // 遍历所有适配器，找到包含目标显示器的输出
    for (UINT adapter_index = 0;; ++adapter_index) {
        IDXGIAdapter* adapter = nullptr;
        ret = dxgi_factory_->EnumAdapters(adapter_index, &adapter);
        if (FAILED(ret)) {
            // 没有更多适配器了
            break;
        }

        // 使用 OnScopeLeave 确保 adapter 在作用域结束时被释放（除非被赋值给成员变量）
        bool adapter_used = false;
        OnScopeLeave([&]() {
            if (!adapter_used && adapter) {
                adapter->Release();
            }
        });

        // 遍历该适配器的所有输出
        for (UINT output_index = 0;; ++output_index) {
            IDXGIOutput* output = nullptr;
            ret = adapter->EnumOutputs(output_index, &output);
            if (FAILED(ret)) {
                // 没有更多输出了
                break;
            }

            // 使用 OnScopeLeave 确保 output 在作用域结束时被释放（除非被赋值给成员变量）
            bool output_used = false;
            OnScopeLeave([&]() {
                if (!output_used && output) {
                    output->Release();
                }
            });

            // 获取输出的描述信息
            DXGI_OUTPUT_DESC output_desc;
            ret = output->GetDesc(&output_desc);
            if (SUCCEEDED(ret) && output_desc.Monitor == monitor) {
                // 找到匹配的显示器
                dxgi_adapter_ = adapter;
                dxgi_output_ = reinterpret_cast<IDXGIOutput1*>(output);
                adapter_used = true;
                output_used = true;
                LogInfo << "Found matching output for window monitor" << VAR(adapter_index) << VAR(output_index);
                return true;
            }
        }
    }

    return false;
}

bool DesktopDupScreencap::init_primary_output()
{
    HRESULT ret = dxgi_factory_->EnumAdapters(0, &dxgi_adapter_);
    if (FAILED(ret)) {
        LogError << "EnumAdapters failed" << VAR(ret);
        return false;
    }

    ret = dxgi_adapter_->EnumOutputs(0, reinterpret_cast<IDXGIOutput**>(&dxgi_output_));
    if (FAILED(ret)) {
        LogError << "EnumOutputs failed" << VAR(ret);
        return false;
    }

    return true;
}

bool DesktopDupScreencap::init_output_duplication()
{
    HRESULT ret = dxgi_output_->DuplicateOutput(d3d_device_, &dxgi_dup_);
    if (FAILED(ret)) {
        LogError << "DuplicateOutput failed" << VAR(ret);
        return false;
    }
    return true;
}

bool DesktopDupScreencap::init_texture(ID3D11Texture2D* raw_texture)
{
    LogFunc;

    if (!d3d_device_ || !raw_texture) {
        LogError << "handle is null" << VAR_VOIDP(d3d_device_) << VAR_VOIDP(raw_texture);
        return false;
    }

    raw_texture->GetDesc(&texture_desc_); // basic info

    texture_desc_.BindFlags = 0;
    texture_desc_.MiscFlags = 0;
    texture_desc_.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    texture_desc_.Usage = D3D11_USAGE_STAGING;

    HRESULT ret = d3d_device_->CreateTexture2D(&texture_desc_, NULL, &readable_texture_);
    if (FAILED(ret)) {
        LogError << "CreateTexture2D failed" << VAR(ret);
        return false;
    }

    return true;
}

void DesktopDupScreencap::uninit()
{
    LogFunc;

    if (readable_texture_) {
        readable_texture_->Release();
        readable_texture_ = nullptr;
    }
    texture_desc_ = { 0 };

    if (dxgi_dup_) {
        dxgi_dup_->Release();
        dxgi_dup_ = nullptr;
    }
    if (dxgi_output_) {
        dxgi_output_->Release();
        dxgi_output_ = nullptr;
    }
    if (dxgi_adapter_) {
        dxgi_adapter_->Release();
        dxgi_adapter_ = nullptr;
    }
    if (dxgi_factory_) {
        dxgi_factory_->Release();
        dxgi_factory_ = nullptr;
    }
    if (d3d_context_) {
        d3d_context_->Release();
        d3d_context_ = nullptr;
    }
    if (d3d_device_) {
        d3d_device_->Release();
        d3d_device_ = nullptr;
    }
}

std::optional<cv::Mat> DesktopDupScreencap::screencap_impl()
{
    if (!d3d_context_ || !dxgi_dup_) {
        LogError << "handle is null" << VAR_VOIDP(d3d_context_) << VAR_VOIDP(dxgi_dup_);
        return std::nullopt;
    }

    HRESULT ret = S_OK;

    DXGI_OUTDUPL_FRAME_INFO frame_info { 0 };
    IDXGIResource* desktop_resource = nullptr;
    ret = dxgi_dup_->AcquireNextFrame(INFINITE, &frame_info, &desktop_resource);
    if (FAILED(ret)) {
        LogError << "AcquireNextFrame failed" << VAR(ret);
        return std::nullopt;
    }
    OnScopeLeave([&]() {
        dxgi_dup_->ReleaseFrame();

        if (desktop_resource) {
            desktop_resource->Release();
            desktop_resource = nullptr;
        }
    });

    ID3D11Texture2D* raw_texture = nullptr;
    ret = desktop_resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&raw_texture));
    if (FAILED(ret)) {
        LogError << "QueryInterface ID3D11Texture2D failed" << VAR(ret);
        return std::nullopt;
    }
    OnScopeLeave([&]() {
        if (raw_texture) {
            raw_texture->Release();
            raw_texture = nullptr;
        }
    });

    // 检测并处理 HDR 纹理
    // 如果纹理是 HDR 格式（如 R16G16B16A16_FLOAT），使用 Direct2D 进行 tone mapping
    auto hdr_result = hdr_tone_mapper_.tone_map(d3d_device_, d3d_context_, raw_texture);
    if (hdr_result) {
        return hdr_result; // HDR 处理成功，直接返回 SDR 结果
    }

    // 非 HDR 格式或 tone mapping 失败，使用原有的 SDR 处理流程
    if (!readable_texture_ && !init_texture(raw_texture)) {
        LogError << "failed to init_texture";
        return std::nullopt;
    }

    d3d_context_->CopyResource(readable_texture_, raw_texture);

    D3D11_MAPPED_SUBRESOURCE mapped { 0 };
    ret = d3d_context_->Map(readable_texture_, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(ret)) {
        LogError << "Map failed" << VAR(ret);
        return std::nullopt;
    }
    OnScopeLeave([&]() { d3d_context_->Unmap(readable_texture_, 0); });

    cv::Mat mat(texture_desc_.Height, texture_desc_.Width, CV_8UC4, mapped.pData, mapped.RowPitch);
    return mat;
}

MAA_CTRL_UNIT_NS_END
