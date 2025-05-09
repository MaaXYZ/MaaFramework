#include "../include/forward.h"

#include <MaaToolkit/MaaToolkitAPI.h>

#include "../include/info.h"
#include "../include/macro.h"
#include "../include/utils.h"
#include "../include/wrapper.h"

Napi::Promise find_adb(Napi::Env env, std::optional<std::string> adb)
{
    using AdbInfo = std::tuple<std::string, std::string, std::string, MaaAdbScreencapMethod, MaaAdbInputMethod, std::string>;
    auto worker = new SimpleAsyncWork<std::optional<std::vector<AdbInfo>>, "find_adb">(env, [adb]() -> std::optional<std::vector<AdbInfo>> {
        auto list = MaaToolkitAdbDeviceListCreate();
        if (adb.has_value()) {
            if (!MaaToolkitAdbDeviceFindSpecified(adb->c_str(), list)) {
                MaaToolkitAdbDeviceListDestroy(list);
                return std::nullopt;
            }
        }
        else {
            if (!MaaToolkitAdbDeviceFind(list)) {
                MaaToolkitAdbDeviceListDestroy(list);
                return std::nullopt;
            }
        }
        auto size = MaaToolkitAdbDeviceListSize(list);
        std::vector<AdbInfo> infos(size);
        for (size_t i = 0; i < size; i++) {
            auto dev = MaaToolkitAdbDeviceListAt(list, i);
            infos[i] = std::make_tuple(
                std::string(MaaToolkitAdbDeviceGetName(dev)),
                std::string(MaaToolkitAdbDeviceGetAdbPath(dev)),
                std::string(MaaToolkitAdbDeviceGetAddress(dev)),
                MaaToolkitAdbDeviceGetScreencapMethods(dev),
                MaaToolkitAdbDeviceGetInputMethods(dev),
                std::string(MaaToolkitAdbDeviceGetConfig(dev)));
        }
        MaaToolkitAdbDeviceListDestroy(list);
        return infos;
    });
    worker->Queue();
    return worker->Promise();
}

Napi::Promise find_desktop(Napi::Env env)
{
    using DesktopInfo = std::tuple<__DesktopHandle*, std::string, std::string>;
    auto worker =
        new SimpleAsyncWork<std::optional<std::vector<DesktopInfo>>, "find_desktop">(env, []() -> std::optional<std::vector<DesktopInfo>> {
            auto list = MaaToolkitDesktopWindowListCreate();

            if (!MaaToolkitDesktopWindowFindAll(list)) {
                MaaToolkitDesktopWindowListDestroy(list);
                return std::nullopt;
            }
            auto size = MaaToolkitDesktopWindowListSize(list);
            std::vector<DesktopInfo> infos(size);
            for (size_t i = 0; i < size; i++) {
                auto dev = MaaToolkitDesktopWindowListAt(list, i);
                infos[i] = std::make_tuple(
                    static_cast<__DesktopHandle*>(MaaToolkitDesktopWindowGetHandle(dev)),
                    std::string(MaaToolkitDesktopWindowGetClassName(dev)),
                    std::string(MaaToolkitDesktopWindowGetWindowName(dev)));
            }
            MaaToolkitDesktopWindowListDestroy(list);
            return infos;
        });
    worker->Queue();
    return worker->Promise();
}

void load_toolkit_find(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context)
{
    BIND(find_adb);
    BIND(find_desktop);
}
