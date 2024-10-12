#include "ControlUnit/AdbControlUnitAPI.h"

#include <meojson/json.hpp>

#include "Manager/ControlUnitMgr.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

using MAA_NS::path;

const char* MaaAdbControlUnitGetVersion()
{
#pragma message("MaaAdbControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaControlUnitHandle MaaAdbControlUnitCreate(
    const char* adb_path,
    const char* adb_serial,
    MaaAdbScreencapMethod screencap_methods,
    MaaAdbInputMethod input_methods,
    const char* config,
    const char* agent_path)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR(adb_path) << VAR(adb_serial) << VAR(screencap_methods) << VAR(input_methods) << VAR(config) << VAR(agent_path);

    auto json_opt = json::parse(config);
    if (!json_opt || !json_opt->is_object()) {
        LogError << "Parse config failed, or not object, invalid config:" << config;
        return nullptr;
    }

    auto unit_mgr = std::make_unique<ControlUnitMgr>(
        path(adb_path),
        adb_serial,
        screencap_methods,
        input_methods,
        json_opt->as_object(),
        path(agent_path));

    return unit_mgr.release();
}

void MaaAdbControlUnitDestroy(MaaControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
