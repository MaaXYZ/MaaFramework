#include "ControlUnit/AdbControlUnitAPI.h"

#include <meojson/json.hpp>

#include "Input/MaatouchInput.h"
#include "Input/MinitouchInput.h"
#include "Input/TapInput.h"
#include "Manager/ControlUnitMgr.h"
#include "Platform/PlatformFactory.h"
#include "Screencap/Encode.h"
#include "Screencap/EncodeToFile.h"
#include "Screencap/FastestWay.h"
#include "Screencap/Minicap/MinicapDirect.h"
#include "Screencap/Minicap/MinicapStream.h"
#include "Screencap/RawByNetcat.h"
#include "Screencap/RawWithGzip.h"
#include "Utils/Logger.h"

using MAA_NS::path;

MaaStringView get_version()
{
#pragma message("MaaAdbControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaControlUnitHandle create_control_unit( //
    MaaStringView adb_path, MaaStringView adb_serial, MaaAdbControllerType type, MaaStringView config,
    MaaStringView agent_path, MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR(adb_path) << VAR(adb_serial) << VAR(type) << VAR(config) << VAR(agent_path) << VAR_VOIDP(callback)
            << VAR_VOIDP(callback_arg);

    std::shared_ptr<TouchInputBase> touch_unit = nullptr;
    std::shared_ptr<KeyInputBase> key_unit = nullptr;
    std::shared_ptr<ScreencapBase> screencap_unit = nullptr;

    auto touch_type = type & MaaAdbControllerType_Touch_Mask;
    auto key_type = type & MaaAdbControllerType_Key_Mask;
    auto screencap_type = type & MaaAdbControllerType_Screencap_Mask;

    std::shared_ptr<MaatouchInput> maatouch_unit = nullptr;

    auto agent_stdpath = path(agent_path);
    auto minitouch_path = agent_stdpath / path("minitouch");
    auto maatouch_path = agent_stdpath / path("maatouch");
    auto minicap_path = agent_stdpath / path("minicap");

    switch (touch_type) {
    case MaaAdbControllerType_Touch_Adb:
        LogInfo << "touch_type: TapTouchInput";
        touch_unit = std::make_shared<TapTouchInput>();
        break;
    case MaaAdbControllerType_Touch_MiniTouch:
        LogInfo << "touch_type: MinitouchInput";
        if (!std::filesystem::exists(minitouch_path)) {
            LogError << "minitouch path not exists" << VAR(minitouch_path);
            return nullptr;
        }
        touch_unit = std::make_shared<MinitouchInput>(minitouch_path);
        break;
    case MaaAdbControllerType_Touch_MaaTouch:
        LogInfo << "touch_type: MaatouchInput";
        if (!std::filesystem::exists(maatouch_path)) {
            LogError << "maatouch path not exists" << VAR(maatouch_path);
            return nullptr;
        }
        if (!maatouch_unit) {
            maatouch_unit = std::make_shared<MaatouchInput>(maatouch_path);
        }
        touch_unit = maatouch_unit;
        break;
    default:
        LogWarn << "Unknown touch input type" << VAR(touch_type);
        break;
    }

    switch (key_type) {
    case MaaAdbControllerType_Key_Adb:
        LogInfo << "key_type: TapKeyInput";
        key_unit = std::make_shared<TapKeyInput>();
        break;
    case MaaAdbControllerType_Key_MaaTouch:
        LogInfo << "key_type: MaatouchInput";
        if (!std::filesystem::exists(maatouch_path)) {
            LogError << "maatouch path not exists" << VAR(maatouch_path);
            return nullptr;
        }
        if (!maatouch_unit) {
            maatouch_unit = std::make_shared<MaatouchInput>(maatouch_path);
        }
        key_unit = maatouch_unit;
        break;
    default:
        LogWarn << "Unknown key input type" << VAR(key_type);
        break;
    }

    switch (screencap_type) {
    case MaaAdbControllerType_Screencap_FastestWay:
        LogInfo << "screencap_type: ScreencapFastestWay";
        if (!std::filesystem::exists(minicap_path)) {
            LogError << "minicap path not exists" << VAR(minicap_path);
            return nullptr;
        }
        screencap_unit = std::make_shared<ScreencapFastestWay>(minicap_path);
        break;
    case MaaAdbControllerType_Screencap_RawByNetcat:
        LogInfo << "screencap_type: ScreencapRawByNetcat";
        screencap_unit = std::make_shared<ScreencapRawByNetcat>();
        break;
    case MaaAdbControllerType_Screencap_RawWithGzip:
        LogInfo << "screencap_type: ScreencapRawWithGzip";
        screencap_unit = std::make_shared<ScreencapRawWithGzip>();
        break;
    case MaaAdbControllerType_Screencap_Encode:
        LogInfo << "screencap_type: ScreencapEncode";
        screencap_unit = std::make_shared<ScreencapEncode>();
        break;
    case MaaAdbControllerType_Screencap_EncodeToFile:
        LogInfo << "screencap_type: ScreencapEncodeToFile";
        screencap_unit = std::make_shared<ScreencapEncodeToFileAndPull>();
        break;
    case MaaAdbControllerType_Screencap_MinicapDirect:
        LogInfo << "screencap_type: MinicapDirect";
        if (!std::filesystem::exists(minicap_path)) {
            LogError << "minicap path not exists" << VAR(minicap_path);
            return nullptr;
        }
        screencap_unit = std::make_shared<MinicapDirect>(minicap_path);
        break;
    case MaaAdbControllerType_Screencap_MinicapStream:
        LogInfo << "screencap_type: MinicapStream";
        if (!std::filesystem::exists(minicap_path)) {
            LogError << "minicap path not exists" << VAR(minicap_path);
            return nullptr;
        }
        screencap_unit = std::make_shared<MinicapStream>(minicap_path);
        break;
    default:
        LogWarn << "Unknown screencap type" << VAR(screencap_type);
        break;
    }

    auto unit_mgr = std::make_unique<ControlUnitMgr>(MAA_NS::path(adb_path), adb_serial, callback, callback_arg);

    unit_mgr->set_touch_input_obj(touch_unit);
    unit_mgr->set_key_input_obj(key_unit);
    unit_mgr->set_screencap_obj(screencap_unit);

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

    auto platform_io = PlatformFactory::create();
    if (!platform_io) {
        LogError << "Create platform io failed";
        return nullptr;
    }
    unit_mgr->set_io(platform_io);

    unit_mgr->set_replacement({
        { "{ADB}", adb_path },
        { "{ADB_SERIAL}", adb_serial },
    });

    return unit_mgr.release();
}

void destroy_control_unit(MaaControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
