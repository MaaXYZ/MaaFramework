#pragma once
#include "controller.h"
#include <memory>
#include <meojson/json.hpp>
#ifdef MAA_JS_WITH_TOOLKIT
#include <MaaToolkit/MaaToolkitAPI.h>

inline std::optional<std::vector<IOSDevice>> find_ios_devices(int64_t timeout, MaaIOSConnectionMethod method, const std::string& config)
{
    auto list = MaaToolkitIOSDeviceListCreate();
    if (!list) {
        return std::nullopt;
    }
    std::unique_ptr<MaaToolkitIOSDeviceList, decltype(&MaaToolkitIOSDeviceListDestroy)> owned(list, MaaToolkitIOSDeviceListDestroy);
    auto options = json::parse(config);
    if (!options || !options->is_object()) {
        return std::nullopt;
    }
    (*options)["timeout_ms"] = timeout;
    const auto text = options->to_string();
    if (!MaaToolkitIOSDeviceFindWithOptions(method, text.c_str(), list)) {
        return std::nullopt;
    }
    std::vector<IOSDevice> result;
    for (MaaSize i = 0; i < MaaToolkitIOSDeviceListSize(list); ++i) {
        auto device = MaaToolkitIOSDeviceListAt(list, i);
        result.emplace_back(
            MaaToolkitIOSDeviceGetName(device),
            MaaToolkitIOSDeviceGetUdid(device),
            MaaToolkitIOSDeviceGetScreencapMethods(device),
            MaaToolkitIOSDeviceGetConnections(device));
    }
    return result;
}

#endif
