#include "ControlUnit/AdbControlUnitAPI.h"

#include <meojson/json.hpp>

#include "Manager/ControlUnitMgr.h"
#include "Manager/InputAgent.h"
#include "Manager/ScreencapAgent.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

using MAA_NS::path;

MaaStringView MaaAdbControlUnitGetVersion()
{
#pragma message("MaaAdbControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaControlUnitHandle MaaAdbControlUnitCreate(
    MaaStringView adb_path,
    MaaStringView adb_serial,
    MaaAdbScreencapMethod screencap_methods,
    MaaAdbInputMethod input_methods,
    MaaStringView config,
    MaaStringView agent_path,
    MaaNotificationCallback callback,
    MaaCallbackTransparentArg callback_arg)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR(adb_path) << VAR(adb_serial) << VAR(screencap_methods) << VAR(input_methods) << VAR(config) << VAR(agent_path)
            << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    auto screencap_unit = std::make_shared<ScreencapAgent>(screencap_methods, agent_path);
    auto input_unit = std::make_shared<InputAgent>(input_methods, agent_path);

    auto unit_mgr = std::make_unique<ControlUnitMgr>(path(adb_path), adb_serial, screencap_unit, input_unit, callback, callback_arg);

    auto json_opt = json::parse(config);
    if (!json_opt) {
        LogError << "Parse config failed, invalid config:" << config;
        return nullptr;
    }
    bool parsed = unit_mgr->parse(*json_opt);
    if (!parsed) {
        LogError << "unit_mgr->parse failed, invalid json:" << *json_opt;
        return nullptr;
    }

    return unit_mgr.release();
}

void MaaAdbControlUnitDestroy(MaaControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
