#include "ControlUnitMgr.h"

#include <meojson/json.hpp>

#include "General/Activity.h"
#include "General/Connection.h"
#include "General/DeviceInfo.h"
#include "General/DeviceList.h"
#include "Input/MaatouchInput.h"
#include "Input/MinitouchInput.h"
#include "Input/TapInput.h"
#include "Platform/PlatformFactory.h"
#include "Screencap/Encode.h"
#include "Screencap/EncodeToFile.h"
#include "Screencap/FastestWay.h"
#include "Screencap/Minicap/MinicapDirect.h"
#include "Screencap/Minicap/MinicapStream.h"
#include "Screencap/RawByNetcat.h"
#include "Screencap/RawWithGzip.h"
#include "Utils/Logger.h"

#pragma message("MaaControlUnit MAA_VERSION: " MAA_VERSION)

MAA_ADB_CTRL_UNIT_NS_BEGIN

bool ControlUnitMgr::parse(const json::value& config)
{
    bool ret = true;

    ret &= connection_ && connection_->parse(config);
    ret &= device_info_ && device_info_->parse(config);
    ret &= activity_ && activity_->parse(config);
    ret &= screencap_ && screencap_->parse(config);
    ret &= touch_input_ && touch_input_->parse(config);
    ret &= key_input_ && key_input_->parse(config);

    return ret;
}

void ControlUnitMgr::set_io(const std::shared_ptr<PlatformIO>& io_ptr)
{
    if (connection_) {
        connection_->set_io(io_ptr);
    }
    if (device_info_) {
        device_info_->set_io(io_ptr);
    }
    if (activity_) {
        activity_->set_io(io_ptr);
    }
    if (touch_input_) {
        touch_input_->set_io(io_ptr);
    }
    if (key_input_) {
        key_input_->set_io(io_ptr);
    }
    if (screencap_) {
        screencap_->set_io(io_ptr);
    }
}

void ControlUnitMgr::set_replacement(const std::map<std::string, std::string>& replacement)
{
    if (connection_) {
        connection_->set_replacement(replacement);
    }
    if (device_info_) {
        device_info_->set_replacement(replacement);
    }
    if (activity_) {
        activity_->set_replacement(replacement);
    }
    if (touch_input_) {
        touch_input_->set_replacement(replacement);
    }
    if (key_input_) {
        key_input_->set_replacement(replacement);
    }
    if (screencap_) {
        screencap_->set_replacement(replacement);
    }
}

std::shared_ptr<DeviceListAPI> create_device_list_obj(MaaStringView adb_path, MaaStringView config)
{
    LogFunc << VAR(adb_path) << VAR(config);

    auto device_list_mgr = std::make_shared<DeviceList>();

    auto json_opt = json::parse(std::string_view(config));
    if (!json_opt) {
        LogError << "Parse config failed, invalid config:" << config;
        return nullptr;
    }
    bool parsed = device_list_mgr->parse(*json_opt);
    if (!parsed) {
        LogError << "Parse json failed, invalid json:" << *json_opt;
        return nullptr;
    }

    auto platform_io = PlatformFactory::create();
    if (!platform_io) {
        LogError << "Create platform io failed";
        return nullptr;
    }
    device_list_mgr->set_io(platform_io);

    device_list_mgr->set_replacement({
        { "{ADB}", adb_path },
    });

    return device_list_mgr;
}

std::shared_ptr<ControlUnitAPI> create_controller_unit(MaaStringView adb_path, MaaStringView adb_serial,
                                                       MaaAdbControllerType type, MaaStringView config,
                                                       MaaStringView agent_path)
{
    LogFunc << VAR(adb_path) << VAR(adb_serial) << VAR(type) << VAR(config) << VAR(agent_path);

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
        LogError << "Unknown touch input type" << VAR(touch_type);
        return nullptr;
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
        LogError << "Unknown key input type" << VAR(key_type);
        return nullptr;
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
        LogError << "Unknown screencap type" << VAR(screencap_type);
        return nullptr;
    }

    auto unit_mgr = std::make_shared<ControlUnitMgr>();

    unit_mgr->set_connection_obj(std::make_shared<Connection>());
    unit_mgr->set_device_info_obj(std::make_shared<DeviceInfo>());
    unit_mgr->set_activity_obj(std::make_shared<Activity>());
    unit_mgr->set_touch_input_obj(touch_unit);
    unit_mgr->set_key_input_obj(key_unit);
    unit_mgr->set_screencap_obj(screencap_unit);

    auto json_opt = json::parse(std::string_view(config));
    if (!json_opt) {
        LogError << "Parse config failed, invalid config:" << config;
        return nullptr;
    }
    bool parsed = unit_mgr->parse(*json_opt);
    if (!parsed) {
        LogError << "Parse json failed, invalid json:" << *json_opt;
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

    return unit_mgr;
}

std::shared_ptr<ConnectionAPI> create_connection(MaaStringView adb_path, MaaStringView adb_serial,
                                                 MaaAdbControllerType type, MaaStringView config)
{
    LogFunc << VAR(adb_path) << VAR(adb_serial) << VAR(type) << VAR(config);

    std::ignore = type;

    auto json_opt = json::parse(std::string_view(config));
    if (!json_opt) {
        LogError << "Parse config failed, invalid config:" << config;
        return nullptr;
    }

    auto connection = std::make_shared<Connection>();

    bool parsed = connection->parse(*json_opt);
    if (!parsed) {
        LogError << "Parse json failed, invalid json:" << *json_opt;
        return nullptr;
    }

    auto platform_io = PlatformFactory::create();
    if (!platform_io) {
        LogError << "Create platform io failed";
        return nullptr;
    }
    connection->set_io(platform_io);
    connection->set_replacement({
        { "{ADB}", adb_path },
        { "{ADB_SERIAL}", adb_serial },
    });

    return connection;
}

std::shared_ptr<DeviceInfoAPI> create_device_info(MaaStringView adb_path, MaaStringView adb_serial,
                                                  MaaAdbControllerType type, MaaStringView config)
{
    LogFunc << VAR(adb_path) << VAR(adb_serial) << VAR(type) << VAR(config);

    std::ignore = type;

    auto json_opt = json::parse(std::string_view(config));
    if (!json_opt) {
        LogError << "Parse config failed, invalid config:" << config;
        return nullptr;
    }

    auto device_info = std::make_shared<DeviceInfo>();

    bool parsed = device_info->parse(*json_opt);
    if (!parsed) {
        LogError << "Parse json failed, invalid json:" << *json_opt;
        return nullptr;
    }

    auto platform_io = PlatformFactory::create();
    if (!platform_io) {
        LogError << "Create platform io failed";
        return nullptr;
    }
    device_info->set_io(platform_io);
    device_info->set_replacement({
        { "{ADB}", adb_path },
        { "{ADB_SERIAL}", adb_serial },
    });

    return device_info;
}

std::shared_ptr<ActivityAPI> create_activity(MaaStringView adb_path, MaaStringView adb_serial,
                                             MaaAdbControllerType type, MaaStringView config)
{
    LogFunc << VAR(adb_path) << VAR(adb_serial) << VAR(type) << VAR(config);

    std::ignore = type;

    auto json_opt = json::parse(std::string_view(config));
    if (!json_opt) {
        LogError << "Parse config failed, invalid config:" << config;
        return nullptr;
    }

    auto activity = std::make_shared<Activity>();

    bool parsed = activity->parse(*json_opt);
    if (!parsed) {
        LogError << "Parse json failed, invalid json:" << *json_opt;
        return nullptr;
    }

    auto platform_io = PlatformFactory::create();
    if (!platform_io) {
        LogError << "Create platform io failed";
        return nullptr;
    }
    activity->set_io(platform_io);
    activity->set_replacement({
        { "{ADB}", adb_path },
        { "{ADB_SERIAL}", adb_serial },
    });

    return activity;
}

std::shared_ptr<TouchInputAPI> create_touch_input(MaaStringView adb_path, MaaStringView adb_serial,
                                                  MaaAdbControllerType type, MaaStringView config,
                                                  MaaStringView agent_path)
{
    LogFunc << VAR(adb_path) << VAR(adb_serial) << VAR(type) << VAR(config) << VAR(agent_path);

    auto json_opt = json::parse(std::string_view(config));
    if (!json_opt) {
        LogError << "Parse config failed, invalid config:" << config;
        return nullptr;
    }

    auto agent_stdpath = path(agent_path);
    auto minitouch_path = agent_stdpath / path("minitouch");
    auto maatouch_path = agent_stdpath / path("maatouch");

    std::shared_ptr<TouchInputBase> touch_unit = nullptr;
    switch (type & MaaAdbControllerType_Touch_Mask) {
    case MaaAdbControllerType_Touch_Adb:
        LogInfo << "touch_type: TapInput";
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
        touch_unit = std::make_shared<MaatouchInput>(maatouch_path);
        break;
    default:
        LogError << "Unknown touch input type" << VAR(type);
        return nullptr;
    }

    bool parsed = touch_unit->parse(*json_opt);
    if (!parsed) {
        LogError << "Parse json failed, invalid json:" << *json_opt;
        return nullptr;
    }

    auto platform_io = PlatformFactory::create();
    if (!platform_io) {
        LogError << "Create platform io failed";
        return nullptr;
    }
    touch_unit->set_io(platform_io);
    touch_unit->set_replacement({
        { "{ADB}", adb_path },
        { "{ADB_SERIAL}", adb_serial },
    });

    return touch_unit;
}

std::shared_ptr<KeyInputAPI> create_key_input(MaaStringView adb_path, MaaStringView adb_serial,
                                              MaaAdbControllerType type, MaaStringView config, MaaStringView agent_path)
{
    LogFunc << VAR(adb_path) << VAR(adb_serial) << VAR(type) << VAR(config) << VAR(agent_path);

    auto json_opt = json::parse(std::string_view(config));
    if (!json_opt) {
        LogError << "Parse config failed, invalid config:" << config;
        return nullptr;
    }
    auto agent_stdpath = path(agent_path);
    auto maatouch_path = agent_stdpath / path("maatouch");

    std::shared_ptr<KeyInputBase> key_unit = nullptr;
    switch (type & MaaAdbControllerType_Key_Mask) {
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
        key_unit = std::make_shared<MaatouchInput>(maatouch_path);
        break;
    default:
        LogError << "Unknown key input type" << VAR(type);
        return nullptr;
    }

    bool parsed = key_unit->parse(*json_opt);
    if (!parsed) {
        LogError << "Parse json failed, invalid json:" << *json_opt;
        return nullptr;
    }

    auto platform_io = PlatformFactory::create();
    if (!platform_io) {
        LogError << "Create platform io failed";
        return nullptr;
    }
    key_unit->set_io(platform_io);
    key_unit->set_replacement({
        { "{ADB}", adb_path },
        { "{ADB_SERIAL}", adb_serial },
    });

    return key_unit;
}

std::shared_ptr<ScreencapAPI> create_screencap(MaaStringView adb_path, MaaStringView adb_serial,
                                               MaaAdbControllerType type, MaaStringView config,
                                               MaaStringView agent_path)
{
    LogFunc << VAR(adb_path) << VAR(adb_serial) << VAR(type) << VAR(config) << VAR(agent_path);

    auto json_opt = json::parse(std::string_view(config));
    if (!json_opt) {
        LogError << "Parse config failed, invalid config:" << config;
        return nullptr;
    }

    auto agent_stdpath = path(agent_path);
    auto minicap_path = agent_stdpath / path("minicap");

    std::shared_ptr<ScreencapBase> screencap_unit = nullptr;
    switch (type & MaaAdbControllerType_Screencap_Mask) {
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
        LogError << "Unknown screencap type" << VAR(type);
        return nullptr;
    }

    bool parsed = screencap_unit->parse(*json_opt);
    if (!parsed) {
        LogError << "Parse json failed, invalid json:" << *json_opt;
        return nullptr;
    }

    auto platform_io = PlatformFactory::create();
    if (!platform_io) {
        LogError << "Create platform io failed";
        return nullptr;
    }
    screencap_unit->set_io(platform_io);
    screencap_unit->set_replacement({
        { "{ADB}", adb_path },
        { "{ADB_SERIAL}", adb_serial },
    });

    return screencap_unit;
}

MAA_ADB_CTRL_UNIT_NS_END
