#pragma once

#include <optional>
#include <string>
#include <vector>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/maa/details/ControllerType.hpp"

namespace maa
{

struct AdbType
    : public details::ControllerType<
          MaaAdbControllerType,
          MaaAdbControllerType_Touch_Mask,
          MaaAdbControllerType_Key_Mask,
          MaaAdbControllerType_Screencap_Mask,
          AdbType>
{
    using ControllerType::ControllerType;
};

struct AdbDevice
{
    std::string adb_path;
    std::string address;
    AdbType type;
    std::string config;
};

namespace AdbDeviceFinder
{

inline coro::Promise<std::shared_ptr<std::vector<AdbDevice>>>
    find(std::optional<std::string> adb = std::nullopt)
{
    using return_type = std::shared_ptr<std::vector<AdbDevice>>;
    if (!(adb.has_value() ? MaaToolkitPostFindDeviceWithAdb(adb.value().c_str())
                          : MaaToolkitPostFindDevice())) {
        return coro::resolve_now<return_type>(nullptr);
    }
    return coro::EventLoop::current()
        ->eval([]() { return MaaToolkitWaitForFindDeviceToComplete(); })
        .then([](auto size) {
            std::vector<AdbDevice> result;
            result.reserve(size);
            for (size_t i = 0; i < size; i++) {
                result.push_back({
                    MaaToolkitGetDeviceAdbPath(i),
                    MaaToolkitGetDeviceAdbSerial(i),
                    MaaToolkitGetDeviceAdbControllerType(i),
                    MaaToolkitGetDeviceAdbConfig(i),
                });
            }
            return std::make_shared<std::vector<AdbDevice>>(std::move(result));
        });
}

}

}
