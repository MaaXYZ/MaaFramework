// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/maa/Exception.hpp"
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

    constexpr static std::tuple<std::string_view, MaaAdbControllerType> touch_index[] = {
        { "adb", MaaAdbControllerType_Touch_Adb },
        { "minitouch", MaaAdbControllerType_Touch_MiniTouch },
        { "maatouch", MaaAdbControllerType_Touch_MaaTouch },
        { "emulator extras", MaaAdbControllerType_Touch_EmulatorExtras },
        { "auto", MaaAdbControllerType_Touch_AutoDetect },
    };

    constexpr static std::tuple<std::string_view, MaaAdbControllerType> key_index[] = {
        { "adb", MaaAdbControllerType_Key_Adb },
        { "maatouch", MaaAdbControllerType_Key_MaaTouch },
        { "emulator extras", MaaAdbControllerType_Key_EmulatorExtras },
        { "auto", MaaAdbControllerType_Key_AutoDetect },
    };

    constexpr static std::tuple<std::string_view, MaaAdbControllerType> screencap_index[] = {
        { "raw by netcat", MaaAdbControllerType_Screencap_RawByNetcat },
        { "raw with gzip", MaaAdbControllerType_Screencap_RawWithGzip },
        { "encode", MaaAdbControllerType_Screencap_Encode },
        { "encode to file", MaaAdbControllerType_Screencap_EncodeToFile },
        { "minicap direct", MaaAdbControllerType_Screencap_MinicapDirect },
        { "minicap stream", MaaAdbControllerType_Screencap_MinicapStream },
        { "emulator extras", MaaAdbControllerType_Screencap_EmulatorExtras },
        { "fastest lossless", MaaAdbControllerType_Screencap_FastestLosslessWay },
        { "fastest", MaaAdbControllerType_Screencap_FastestWay },
    };
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
    if (adb.has_value()) {
        if (!MaaToolkitPostFindDeviceWithAdb(adb.value().c_str())) {
            throw FunctionFailed("MaaToolkitPostFindDeviceWithAdb");
        }
    }
    else {
        if (!MaaToolkitPostFindDevice()) {
            throw FunctionFailed("MaaToolkitPostFindDevice");
        }
    }
    auto size = co_await coro::EventLoop::current()->eval(
        []() { return MaaToolkitWaitForFindDeviceToComplete(); });
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
