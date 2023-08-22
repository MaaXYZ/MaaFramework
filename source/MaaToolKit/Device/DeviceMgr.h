#pragma once

#include "Conf/Conf.h"
#include "DeviceAPI.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <ostream>

#include <meojson/json.hpp>

MAA_TOOLKIT_DEVICE_NS_BEGIN

std::ostream& operator<<(std::ostream& os, const Device& device);

class DeviceMgr : public MaaToolKitDeviceMgrAPI
{

public:
    virtual ~DeviceMgr() noexcept override = default;

public: // from MaaToolKitDeviceMgrAPI
    virtual size_t find_device(std::string_view specified_adb = std::string_view()) override final;
    virtual const std::vector<Device>& get_devices() override final { return devices_; };

protected:
    virtual std::vector<Device> find_device_impl(std::string_view specified_adb) = 0;

protected:
    std::vector<std::string> request_adb_serials(const std::filesystem::path& adb_path,
                                                 const json::value& adb_config) const;

private:
    std::vector<Device> devices_;
};

MAA_TOOLKIT_DEVICE_NS_END
