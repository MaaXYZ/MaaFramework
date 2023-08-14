#include "DeviceMgrLinux.h"

MAA_TOOLKIT_DEVICE_NS_BEGIN

size_t DeviceMgrLinux::find_device()
{
    // TODO
    return 0;
}

std::string_view DeviceMgrLinux::device_name(size_t index) const
{
    std::ignore = index;
    return std::string_view();
}

std::string_view DeviceMgrLinux::device_adb_path(size_t index) const
{
    std::ignore = index;
    return std::string_view();
}

std::string_view DeviceMgrLinux::device_adb_serial(size_t index) const
{
    std::ignore = index;
    return std::string_view();
}

MaaAdbControllerType DeviceMgrLinux::device_adb_controller_type(size_t index) const
{
    std::ignore = index;
    return MaaAdbControllerType();
}

std::string_view DeviceMgrLinux::device_adb_config(size_t index) const
{
    std::ignore = index;
    return std::string_view();
}

MAA_TOOLKIT_DEVICE_NS_END
