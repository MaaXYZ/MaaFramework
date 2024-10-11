#ifdef _WIN32

#include "Utils/GpuOption.h"
#include "Utils/SafeWindows.hpp"

#include <initguid.h>

#include <cfgmgr32.h>
#include <d3d12.h>
#include <devpkey.h>
#include <devpropdef.h>
#include <dxgi1_6.h>

#include "Utils/Logger.h"

MAA_NS_BEGIN

std::optional<std::wstring> adapter_instance_path(LUID luid)
{
    DISPLAYCONFIG_ADAPTER_NAME req {};
    req.header.size = sizeof(DISPLAYCONFIG_ADAPTER_NAME);
    req.header.adapterId = luid;
    req.header.id = 0;
    req.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;

    LONG adpname = DisplayConfigGetDeviceInfo(&req.header);
    std::ignore = adpname;

    ULONG size = 0;
    DEVPROPTYPE type {};
    CONFIGRET err = CM_Get_Device_Interface_PropertyW(req.adapterDevicePath, &DEVPKEY_Device_InstanceId, &type, nullptr, &size, 0);
    if (err != CR_BUFFER_SMALL) {
        return std::nullopt;
    }
    if (type != DEVPROP_TYPE_STRING) {
        return std::nullopt;
    }

    std::vector<BYTE> buf(size);
    err = CM_Get_Device_Interface_PropertyW(req.adapterDevicePath, &DEVPKEY_Device_InstanceId, &type, buf.data(), &size, 0);
    if (err != CR_SUCCESS) {
        return std::nullopt;
    }

    std::wstring result(reinterpret_cast<const wchar_t*>(buf.data()), size / 2 - 1);
    LogTrace << VAR(result);
    return result;
}

std::optional<int32_t> perfer_gpu()
{
    IDXGIFactory4* dxgi_factory = nullptr;
    OnScopeLeave([&]() {
        if (dxgi_factory) {
            dxgi_factory->Release();
            dxgi_factory = nullptr;
        }
    });

    HRESULT ret = CreateDXGIFactory2(0, __uuidof(IDXGIFactory4), reinterpret_cast<void**>(&dxgi_factory));
    if (FAILED(ret)) {
        LogError << "CreateDXGIFactory2 failed" << VAR(ret);
        return false;
    }

    for (UINT i = 0;; ++i) {
        IDXGIAdapter1* dxgi_adapter = nullptr;
        OnScopeLeave([&]() {
            if (dxgi_adapter) {
                dxgi_adapter->Release();
                dxgi_adapter = nullptr;
            }
        });

        HRESULT hr = dxgi_factory->EnumAdapters1(i, &dxgi_adapter);
        if (hr == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        else if (FAILED(hr)) {
            LogError << "EnumAdapters1 failed" << VAR(hr);
            continue;
        }

        DXGI_ADAPTER_DESC1 desc {};
        hr = dxgi_adapter->GetDesc1(&desc);
        if (FAILED(hr)) {
            LogError << "GetDesc1 failed" << VAR(hr);
            continue;
        }

        auto instance_path = adapter_instance_path(desc.AdapterLuid);
    }

    return std::nullopt;
}

MAA_NS_END

#endif // _WIN32
