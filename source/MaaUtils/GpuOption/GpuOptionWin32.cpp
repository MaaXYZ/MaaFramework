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
#include "Utils/StringMisc.hpp"

MAA_NS_BEGIN

std::optional<std::wstring> adapter_instance_path(LUID luid)
{
    LogTrace;

    DISPLAYCONFIG_ADAPTER_NAME req {};
    req.header.size = sizeof(DISPLAYCONFIG_ADAPTER_NAME);
    req.header.adapterId = luid;
    req.header.id = 0;
    req.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;

    DisplayConfigGetDeviceInfo(&req.header);

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
    LogDebug << VAR(result);
    return result;
}

SYSTEMTIME gpu_driver_date(std::wstring_view instance_path)
{
    LogTrace;

    DEVINST devinst = 0;
    CONFIGRET err = CM_Locate_DevNodeW(&devinst, const_cast<DEVINSTID_W>(instance_path.data()), CM_LOCATE_DEVNODE_NORMAL);
    if (err != CR_SUCCESS) {
        LogError << "CM_Locate_DevNodeW failed" << VAR(err);
        return {};
    }

    DEVPROPTYPE prop_type = DEVPROP_TYPE_FILETIME;
    std::vector<BYTE> date_buffer(sizeof(FILETIME));
    ULONG size = sizeof(FILETIME);
    err = CM_Get_DevNode_PropertyW(devinst, &DEVPKEY_Device_DriverDate, &prop_type, date_buffer.data(), &size, 0);
    if (err != CR_SUCCESS) {
        LogError << "CM_Get_DevNode_PropertyW failed" << VAR(err);
        return {};
    }
    if (prop_type != DEVPROP_TYPE_FILETIME) {
        LogError << "CM_Get_DevNode_PropertyW failed" << VAR(prop_type);
        return {};
    }

    FILETIME file_time = *reinterpret_cast<FILETIME*>(date_buffer.data());
    SYSTEMTIME system_time = {};
    if (!FileTimeToSystemTime(&file_time, &system_time)) {
        LogError << "FileTimeToSystemTime failed" << GetLastError();
        return {};
    }

    LogDebug << VAR(system_time.wYear) << VAR(system_time.wMonth) << VAR(system_time.wDay);
    return system_time;
}

bool is_indirect_display_adapter(std::wstring_view instance_path)
{
    LogTrace;

    HKEY key = nullptr;
    auto close_key = [&]() {
        if (key) {
            RegCloseKey(key);
            key = nullptr;
        }
    };
    OnScopeLeave(close_key);

    std::wstring sub_key = L"SYSTEM\\CurrentControlSet\\Enum\\" + std::wstring(instance_path);

    LONG ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, sub_key.c_str(), 0, KEY_READ, &key);
    if (ret != ERROR_SUCCESS) {
        LogError << "RegOpenKeyExW failed" << VAR(ret);
        return false;
    }

    DWORD size = 0;
    DWORD type = 0;
    ret = RegQueryValueExW(key, L"UpperFilters", nullptr, &type, nullptr, &size);

    if (ret != ERROR_SUCCESS || size == 0) {
        // if no UpperFilters value, it's a direct display adapter
        LogDebug << "RegQueryValueExW 1 failed" << VAR(ret) << VAR(type) << VAR(size);
        return false;
    }

    std::vector<BYTE> data(size);
    ret = RegQueryValueExW(key, L"UpperFilters", nullptr, &type, data.data(), &size);
    if (ret != ERROR_SUCCESS || size == 0) {
        // if no UpperFilters value, it's a direct display adapter
        LogDebug << "RegQueryValueExW 2 failed" << VAR(ret) << VAR(type) << VAR(size);
        return false;
    }

    close_key();

    std::wstring value(reinterpret_cast<wchar_t*>(data.data()), size / 2 - 1);

    bool indirect = value.find(L"IndirectKmd") != std::wstring::npos;
    LogDebug << VAR(value) << VAR(sub_key) << VAR(indirect);
    return indirect;
}

std::optional<int> perfer_gpu()
{
    LogTrace;

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

    for (UINT adapter_index = 0;; ++adapter_index) {
        IDXGIAdapter1* dxgi_adapter = nullptr;
        OnScopeLeave([&]() {
            if (dxgi_adapter) {
                dxgi_adapter->Release();
                dxgi_adapter = nullptr;
            }
        });

        HRESULT hr = dxgi_factory->EnumAdapters1(adapter_index, &dxgi_adapter);
        if (hr == DXGI_ERROR_NOT_FOUND) {
            LogDebug << "EnumAdapters1 DXGI_ERROR_NOT_FOUND" << VAR(adapter_index);
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
        std::wstring adapter_desc(desc.Description);
        LogInfo << VAR(adapter_index) << VAR(adapter_desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            LogWarn << "software adapter, skip" << VAR(adapter_index) << VAR(adapter_desc);
            continue;
        }
        if (desc.DedicatedVideoMemory < 1024 * 1024 * 1024) {
            LogWarn << "adapter has less than 1GB video memory, skip" << VAR(adapter_index) << VAR(adapter_desc)
                    << VAR(desc.DedicatedVideoMemory);
            continue;
        }

        hr = D3D12CreateDevice(dxgi_adapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr);
        if (FAILED(hr)) {
            LogWarn << "adapter not support D3D12 with D3D_FEATURE_LEVEL_12_0, skip" << VAR(adapter_index) << VAR(adapter_desc);
            continue;
        }

        auto instance_path_opt = adapter_instance_path(desc.AdapterLuid);
        if (!instance_path_opt) {
            LogError << "adapter_instance_path failed";
            continue;
        }
        const std::wstring& instance_path = *instance_path_opt;

        auto driver_date = gpu_driver_date(instance_path);
        // reject drivers that predates DirectML (released alongside with Windows 10 1903, i.e. 2019-05-21)
        if (driver_date.wYear < 2019 || (driver_date.wYear == 2019 && driver_date.wMonth < 5)
            || (driver_date.wYear == 2019 && driver_date.wMonth == 5 && driver_date.wDay < 21)) {
            LogWarn << "driver date is too old, skip" << VAR(adapter_index) << VAR(adapter_desc) << VAR(instance_path)
                    << VAR(driver_date.wYear) << VAR(driver_date.wMonth) << VAR(driver_date.wDay);
            continue;
        }

        if (is_indirect_display_adapter(instance_path)) {
            LogWarn << "virtual adapters (streaming/RDP), skip" << VAR(adapter_index) << VAR(adapter_desc) << VAR(instance_path);
            continue;
        }

        LogInfo << "prefer adapter found" << VAR(adapter_index) << VAR(adapter_desc) << VAR(instance_path);

        return adapter_index;
    }

    LogInfo << "no prefer adapter found";
    return std::nullopt;
}

MAA_NS_END

#endif // _WIN32
