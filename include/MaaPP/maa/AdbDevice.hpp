#pragma once

#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"

namespace maa
{

struct AdbDevice
{
    std::string adb_path;
    std::string address;
    MaaAdbControllerType type;
    std::string config;
};

namespace AdbDeviceFinder
{

inline maa::coro::Promise<std::shared_ptr<std::vector<AdbDevice>>> find()
{
    if (!MaaToolkitPostFindDevice()) {
        co_return nullptr;
    }
    auto waiter = maa::coro::Promise<MaaSize>();
    std::thread([waiter]() {
        auto size = MaaToolkitWaitForFindDeviceToComplete();
        maa::coro::EventLoop::current()->defer([waiter, size]() { waiter.resolve(size); });
    }).detach();
    auto size = co_await waiter;
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
    co_return std::make_shared<std::vector<AdbDevice>>(std::move(result));
}

}

}
