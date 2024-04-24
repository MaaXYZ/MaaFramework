#pragma once

#include <optional>
#include <string>
#include <thread>
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
          MaaAdbControllerType_Screencap_Mask>
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

inline maa::coro::Promise<std::shared_ptr<std::vector<AdbDevice>>> find()
{
    if (!MaaToolkitPostFindDevice()) {
        co_return nullptr;
    }
    auto size = co_await maa::coro::EventLoop::current()->eval(
        []() { return MaaToolkitWaitForFindDeviceToComplete(); });
    std::vector<AdbDevice> result;
    result.reserve(size);
    for (size_t i = 0; i < size; i++) {
        result.push_back({
            MaaToolkitGetDeviceAdbPath(i),
            MaaToolkitGetDeviceAdbSerial(i),
            { MaaToolkitGetDeviceAdbControllerType(i) },
            MaaToolkitGetDeviceAdbConfig(i),
        });
    }
    co_return std::make_shared<std::vector<AdbDevice>>(std::move(result));
}

}

}
