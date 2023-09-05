#if defined(__APPLE__)

#include "DeviceMgrMacOS.h"

#include <tuple> // for std::ignore

MAA_TOOLKIT_DEVICE_NS_BEGIN

std::vector<Device> DeviceMgrMacOS::find_device_impl()
{
    return {};
}

std::vector<Device> DeviceMgrMacOS::find_device_with_adb_impl(std::string_view adb_path)
{
    std::ignore = adb_path;
    return {};
}

MAA_TOOLKIT_DEVICE_NS_END

#endif
