#include "DeviceMgr.h"

#include "Utils/Logger.h"
#include "ControlUnit/ControlUnitAPI.h"

MAA_TOOLKIT_DEVICE_NS_BEGIN

std::ostream& operator<<(std::ostream& os, const Device& device)
{
    os << VAR_RAW(device.name) << VAR_RAW(device.adb_path) << VAR_RAW(json::array(device.adb_serials))
       << VAR_RAW(device.adb_controller_type) << VAR_RAW(device.adb_config);
    return os;
}

size_t DeviceMgr::find_device(std::string_view specified_adb)
{
    LogFunc << VAR(specified_adb);

    devices_ = find_device_impl(specified_adb);
    return devices_.size();
}

std::vector<std::string> DeviceMgr::request_adb_serials(const std::filesystem::path& adb_path,
                                                        const json::value& adb_config) const
{
    LogFunc << VAR(adb_path);

    std::string str_adb = path_to_utf8_string(adb_path);
    std::string str_config = adb_config.to_string();
    auto mgr = MAA_CTRL_UNIT_NS::create_adb_device_list_obj(str_adb.data(), str_config.c_str());
    if (!mgr) {
        LogError << "Failed to create_adb_device_list_obj" << VAR(str_adb) << VAR(str_config);
        return {};
    }

    auto devices_opt = mgr->request_devices();
    if (!devices_opt) {
        return {};
    }

    return *devices_opt;
}

MAA_TOOLKIT_DEVICE_NS_END
