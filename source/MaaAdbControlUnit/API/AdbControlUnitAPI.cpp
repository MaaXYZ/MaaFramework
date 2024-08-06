#include "ControlUnit/AdbControlUnitAPI.h"

#include <meojson/json.hpp>

#include "Manager/ControlUnitMgr.h"
#include "Manager/InputManager.h"
#include "Manager/ScreencapManager.h"
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

    ScreencapManager::MethodSet screencap_method_set;
    if (screencap_methods & MaaAdbScreencapMethod_EncodeToFileAndPull) {
        screencap_method_set.emplace(ScreencapManager::Method::EncodeToFileAndPull);
    }
    if (screencap_methods & MaaAdbScreencapMethod_Encode) {
        screencap_method_set.emplace(ScreencapManager::Method::Encode);
    }
    if (screencap_methods & MaaAdbScreencapMethod_RawWithGzip) {
        screencap_method_set.emplace(ScreencapManager::Method::RawWithGzip);
    }
    if (screencap_methods & MaaAdbScreencapMethod_RawByNetcat) {
        screencap_method_set.emplace(ScreencapManager::Method::RawByNetcat);
    }
    if (screencap_methods & MaaAdbScreencapMethod_MinicapDirect) {
        screencap_method_set.emplace(ScreencapManager::Method::MinicapDirect);
    }
    if (screencap_methods & MaaAdbScreencapMethod_MinicapStream) {
        screencap_method_set.emplace(ScreencapManager::Method::MinicapStream);
    }
    if (screencap_methods & MaaAdbScreencapMethod_EmulatorExtras) {
        screencap_method_set.emplace(ScreencapManager::Method::MumuExternalRendererIpc);
        // TODO: add LDPlayer and more...
    }

    InputManager::MethodVec input_method_vec;
    if (input_methods & MaaAdbInputMethod_AdbShell) {
        input_method_vec.emplace_back(InputManager::Method::AdbShell);
    }
    if (input_methods & MaaAdbInputMethod_MinitouchAndAdbKey) {
        input_method_vec.emplace_back(InputManager::Method::MinitouchAndAdbKey);
    }
    if (input_methods & MaaAdbInputMethod_Maatouch) {
        input_method_vec.emplace_back(InputManager::Method::Maatouch);
    }
    if (input_methods & MaaAdbInputMethod_EmulatorExtras) {
        input_method_vec.emplace_back(InputManager::Method::MumuExternalRendererIpc);
        // TODO: add LDPlayer and more...
    }

    auto screencap_unit = std::make_shared<ScreencapManager>(screencap_method_set, agent_path);
    auto input_unit = std::make_shared<InputManager>(input_method_vec, agent_path);

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
