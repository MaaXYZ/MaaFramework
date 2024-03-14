#if defined(__linux__)

#include "DeviceMgrLinux.h"

#include <tuple> // for std::ignore

MAA_TOOLKIT_NS_BEGIN

std::vector<Device> DeviceMgrLinux::find_device_impl()
{
    return {};
}

std::vector<Device> DeviceMgrLinux::find_device_with_adb_impl(std::filesystem::path adb_path)
{
    std::ignore = adb_path;
    return {};
}

MAA_TOOLKIT_NS_END

#endif