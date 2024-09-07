#include "Impl/PIRuntime.h"
#include "Utils/Platform.h"

#include <MaaFramework/MaaAPI.h>

MAA_PI_NS_BEGIN

bool PIRuntime::bind(MaaPIData* _data, MaaPIConfig* _config)
{
    auto data = dynamic_cast<PIData*>(_data);
    auto config = dynamic_cast<PIConfig*>(_config);
    if (!(data && config)) {
        return false;
    }
    data_ = data;
    config_ = config;
    return true;
}

bool PIRuntime::setup(MaaNotificationCallback cb, void* cb_arg)
{
    auto discard = [&]() {
        if (tasker_) {
            MaaTaskerDestroy(tasker_);
        }

        if (controller_) {
            MaaControllerDestroy(controller_);
        }

        if (resource_) {
            MaaResourceDestroy(resource_);
        }

        controller_ = nullptr;
        resource_ = nullptr;
        tasker_ = nullptr;
    };

    if (auto adb_cfg = std::get_if<PIConfig::AdbConfigInfo>(&config_->controller_info_)) {
        if (auto adb = std::get_if<PIData::ControllerInfo::AdbInfo>(&config_->controller_->info)) {
            controller_ = MaaAdbControllerCreate(
                adb_cfg->adb_path.c_str(),
                adb_cfg->address.c_str(),
                adb->screencap,
                adb->input,
                (adb->config | adb_cfg->config).to_string().c_str(),
                path_to_utf8_string(path(data_->resource_root_) / "MaaAgentBinary").c_str(),
                cb,
                cb_arg);
        }
    }
    else if (auto desktop_cfg = std::get_if<PIConfig::DesktopConfigInfo>(&config_->controller_info_)) {
        if (auto desktop = std::get_if<PIData::ControllerInfo::DesktopInfo>(&config_->controller_->info)) {
            controller_ = MaaWin32ControllerCreate(desktop_cfg->hwnd, desktop->screencap, desktop->input, cb, cb_arg);
        }
    }

    if (!controller_) {
        discard();
        return false;
    }

    resource_ = MaaResourceCreate(cb, cb_arg);
    tasker_ = MaaTaskerCreate(cb, cb_arg);

    MaaTaskerBindController(tasker_, controller_);
    MaaTaskerBindResource(tasker_, resource_);

    auto ctrlId = MaaControllerPostConnection(controller_);
    std::vector<MaaResId> resIds;
    for (const auto& path : config_->resource_->paths) {
        resIds.push_back(MaaResourcePostPath(resource_, path_to_utf8_string(path).c_str()));
    }

    if (MaaControllerWait(controller_, ctrlId) != MaaStatus_Success) {
        discard();
        return false;
    }

    for (auto id : resIds) {
        if (MaaResourceWait(resource_, id) != MaaStatus_Success) {
            discard();
            return false;
        }
    }

    if (!MaaTaskerInited(tasker_)) {
        discard();
        return false;
    }

    return true;
}

MaaController* PIRuntime::get_controller()
{
    return controller_;
}

MaaResource* PIRuntime::get_resource()
{
    return resource_;
}

MaaTasker* PIRuntime::get_tasker()
{
    return tasker_;
}

MAA_PI_NS_END
