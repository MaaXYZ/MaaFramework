#include "ControlUnitMgr.h"

#include "General/Activity.h"
#include "General/Connection.h"
#include "General/DeviceInfo.h"
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
#include "Utils/Logger.hpp"

MAA_CTRL_UNIT_NS_BEGIN

void ControlUnitMgr::set_adb(const std::string& adb_path, const std::string& adb_serial)
{
    std::map<std::string, std::string> replacement {
        { "{ADB}", adb_path },
        { "{ADB_SERIAL}", adb_serial },
    };

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

std::shared_ptr<ControlUnitAPI> create_controller_unit(MaaAdbControllerType type, MaaJsonString config)
{
    LogFunc << VAR(type) << VAR(config);

    std::shared_ptr<TouchInputBase> touch_unit = nullptr;
    std::shared_ptr<KeyInputBase> key_unit = nullptr;
    std::shared_ptr<ScreencapBase> screencap_unit = nullptr;

    auto touch_type = type & MaaAdbControllerType_Touch_Mask;
    auto key_type = type & MaaAdbControllerType_Key_Mask;
    auto screencap_type = type & MaaAdbControllerType_Screencap_Mask;

    std::shared_ptr<MaatouchInput> maatouch_unit = nullptr;

    switch (touch_type) {
    case MaaAdbControllerType_Touch_Adb:
        LogInfo << "touch_type: TapInput";
        touch_unit = std::make_shared<TapTouchInput>();
        break;
    case MaaAdbControllerType_Touch_MiniTouch:
        LogInfo << "touch_type: MinitouchInput";
        touch_unit = std::make_shared<MinitouchInput>();
        break;
    case MaaAdbControllerType_Touch_MaaTouch:
        LogInfo << "touch_type: MaatouchInput";
        if (!maatouch_unit) {
            maatouch_unit = std::make_shared<MaatouchInput>();
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
    case MaaAdbControllerType_Touch_MaaTouch:
        LogInfo << "key_type: TapKeyInput";
        if (!maatouch_unit) {
            maatouch_unit = std::make_shared<MaatouchInput>();
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
        screencap_unit = std::make_shared<ScreencapFastestWay>();
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
        screencap_unit = std::make_shared<MinicapDirect>();
        break;
    case MaaAdbControllerType_Screencap_MinicapStream:
        LogInfo << "screencap_type: MinicapStream";
        screencap_unit = std::make_shared<MinicapStream>();
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

    return unit_mgr;
}

std::shared_ptr<ConnectionAPI> create_connection(MaaAdbControllerType type, MaaJsonString config)
{
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

    return connection;
}

std::shared_ptr<DeviceInfoAPI> create_device_info(MaaAdbControllerType type, MaaJsonString config)
{
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

    return device_info;
}

std::shared_ptr<ActivityAPI> create_activity(MaaAdbControllerType type, MaaJsonString config)
{
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

    return activity;
}

std::shared_ptr<TouchInputAPI> create_touch_input(MaaAdbControllerType type, MaaJsonString config)
{
    auto json_opt = json::parse(std::string_view(config));
    if (!json_opt) {
        LogError << "Parse config failed, invalid config:" << config;
        return nullptr;
    }

    std::shared_ptr<TouchInputBase> touch_unit = nullptr;
    switch (type & MaaAdbControllerType_Touch_Mask) {
    case MaaAdbControllerType_Touch_Adb:
        LogInfo << "touch_type: TapInput";
        touch_unit = std::make_shared<TapTouchInput>();
        break;
    case MaaAdbControllerType_Touch_MiniTouch:
        LogInfo << "touch_type: MinitouchInput";
        touch_unit = std::make_shared<MinitouchInput>();
        break;
    case MaaAdbControllerType_Touch_MaaTouch:
        LogInfo << "touch_type: MaatouchInput";
        touch_unit = std::make_shared<MaatouchInput>();
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

    return touch_unit;
}

std::shared_ptr<KeyInputAPI> create_key_input(MaaAdbControllerType type, MaaJsonString config)
{
    auto json_opt = json::parse(std::string_view(config));
    if (!json_opt) {
        LogError << "Parse config failed, invalid config:" << config;
        return nullptr;
    }

    std::shared_ptr<KeyInputBase> key_unit = nullptr;
    switch (type & MaaAdbControllerType_Key_Mask) {
    case MaaAdbControllerType_Key_Adb:
        LogInfo << "key_type: TapKeyInput";
        key_unit = std::make_shared<TapKeyInput>();
        break;
    case MaaAdbControllerType_Touch_MaaTouch:
        LogInfo << "key_type: TapKeyInput";
        key_unit = std::make_shared<MaatouchInput>();
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

    return key_unit;
}

std::shared_ptr<ScreencapAPI> create_screencap(MaaAdbControllerType type, MaaJsonString config)
{
    auto json_opt = json::parse(std::string_view(config));
    if (!json_opt) {
        LogError << "Parse config failed, invalid config:" << config;
        return nullptr;
    }

    std::shared_ptr<ScreencapBase> screencap_unit = nullptr;
    switch (type & MaaAdbControllerType_Screencap_Mask) {
    case MaaAdbControllerType_Screencap_FastestWay:
        LogInfo << "screencap_type: ScreencapFastestWay";
        screencap_unit = std::make_shared<ScreencapFastestWay>();
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
        screencap_unit = std::make_shared<MinicapDirect>();
        break;
    case MaaAdbControllerType_Screencap_MinicapStream:
        LogInfo << "screencap_type: MinicapStream";
        screencap_unit = std::make_shared<MinicapStream>();
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

    return screencap_unit;
}

MAA_CTRL_UNIT_NS_END
