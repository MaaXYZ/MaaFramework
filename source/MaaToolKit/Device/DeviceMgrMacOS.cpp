#include "DeviceMgrMacOS.h"

#include <tuple> // for std::ignore

MAA_TOOLKIT_DEVICE_NS_BEGIN

size_t DeviceMgrMacOS::find_device(std::string_view adb_path)
{
    // TODO
    std::ignore = adb_path;
    return 0;
}

std::string_view DeviceMgrMacOS::device_name(size_t index) const
{
    std::ignore = index;
    return std::string_view();
}

std::string_view DeviceMgrMacOS::device_adb_path(size_t index) const
{
    std::ignore = index;
    return std::string_view();
}

std::string_view DeviceMgrMacOS::device_adb_serial(size_t index) const
{
    std::ignore = index;
    return std::string_view();
}

MaaAdbControllerType DeviceMgrMacOS::device_adb_controller_type(size_t index) const
{
    std::ignore = index;
    return MaaAdbControllerType();
}

std::string_view DeviceMgrMacOS::device_adb_config(size_t index) const
{
    std::ignore = index;
    return std::string_view();
}

MAA_TOOLKIT_DEVICE_NS_END
