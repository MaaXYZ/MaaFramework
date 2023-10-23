#include "DeviceMgr.h"

#include "ControlUnit/AdbControlUnitAPI.h"
#include "Utils/Logger.h"

MAA_TOOLKIT_DEVICE_NS_BEGIN

std::ostream& operator<<(std::ostream& os, const DeviceMgr::Emulator& emulator)
{
    os << VAR_RAW(emulator.name) << VAR_RAW(emulator.process);
    return os;
}

size_t DeviceMgr::find_device()
{
    LogFunc;

    devices_ = find_device_impl();

    LogInfo << VAR(devices_);
    return devices_.size();
}

size_t DeviceMgr::find_device_with_adb(std::string_view adb_path)
{
    LogFunc << VAR(adb_path);

    devices_ = find_device_with_adb_impl(adb_path);

    LogInfo << VAR(devices_);
    return devices_.size();
}

std::vector<std::string> DeviceMgr::request_adb_serials(const std::filesystem::path& adb_path,
                                                        const json::value& adb_config) const
{
    LogFunc << VAR(adb_path);

#ifdef WITH_ADB_CONTROLLER

    std::string str_adb = path_to_utf8_string(adb_path);
    std::string str_config = adb_config.to_string();
    auto mgr = MAA_ADB_CTRL_UNIT_NS::create_device_list_obj(str_adb.c_str(), str_config.c_str());
    if (!mgr) {
        LogError << "Failed to create_device_list_obj" << VAR(str_adb) << VAR(str_config);
        return {};
    }

    auto devices_opt = mgr->request_devices();
    if (!devices_opt) {
        return {};
    }

    return *devices_opt;

#else

    std::ignore = adb_config;
    return {};

#endif
}

bool DeviceMgr::request_adb_connect(const std::filesystem::path& adb_path, const std::string& serial,
                                    const json::value& adb_config) const
{
    LogFunc << VAR(adb_path) << VAR(serial);

#ifdef WITH_ADB_CONTROLLER

    std::string str_adb = path_to_utf8_string(adb_path);
    std::string str_config = adb_config.to_string();
    auto mgr = MAA_ADB_CTRL_UNIT_NS::create_connection(str_adb.c_str(), serial.c_str(), 0, str_config.c_str());
    if (!mgr) {
        LogError << "Failed to create_connection" << VAR(str_adb) << VAR(serial) << VAR(str_config);
        return false;
    }

    return mgr->connect();

#else

    std::ignore = adb_config;
    return false;

#endif
}

std::vector<std::string> DeviceMgr::check_available_adb_serials(const std::filesystem::path& adb_path,
                                                                const std::vector<std::string>& serials,
                                                                const json::value& adb_config) const
{
    std::vector<std::string> available;
    for (const std::string& ser : serials) {
        if (!request_adb_connect(adb_path, ser, adb_config)) {
            continue;
        }
        available.emplace_back(ser);
    }
    return available;
}

MaaAdbControllerType DeviceMgr::check_adb_controller_type(const std::filesystem::path& adb_path,
                                                          const std::string& adb_serial,
                                                          const json::value& adb_config) const
{
    std::ignore = adb_path;
    std::ignore = adb_serial;
    std::ignore = adb_config;

    // TODO: 判断触控方式是否支持
    constexpr MaaAdbControllerType kInputType = MaaAdbControllerType_Input_Preset_Maatouch;

#ifdef MAA_DEBUG
    constexpr MaaAdbControllerType kScreencapType = MaaAdbControllerType_Screencap_RawWithGzip;
#else
    // TODO: speed test
    constexpr MaaAdbControllerType kScreencapType = MaaAdbControllerType_Screencap_FastestWay;
#endif

    return kInputType | kScreencapType;
}

MAA_TOOLKIT_DEVICE_NS_END
