#include "MaaToolKit/Device/MaaToolKitDevice.h"

#include "Device/DeviceMgrLinux.h"
#include "Device/DeviceMgrMacOS.h"
#include "Device/DeviceMgrWin32.h"
#include "Utils/Logger.h"

#if defined(_WIN32)
static MaaToolKitDeviceMgrAPI& device_mgr = MAA_TOOLKIT_DEVICE_NS::DeviceMgrWin32::get_instance();
#elif defined(__linux__)
static MaaToolKitDeviceMgrAPI& device_mgr = MAA_TOOLKIT_DEVICE_NS::DeviceMgrLinux::get_instance();
#elif defined(__APPLE__)
static MaaToolKitDeviceMgrAPI& device_mgr = MAA_TOOLKIT_DEVICE_NS::DeviceMgrMacOS::get_instance();
#endif

MaaSize MaaToolKitFindDevice()
{
    LogFunc;

    return device_mgr.find_device();
}

MaaSize MaaToolKitFindDeviceWithAdb(MaaStringView adb_path)
{
    LogFunc;

    return device_mgr.find_device_with_adb(adb_path);
}

MaaStringView MaaToolKitGetDeviceName(MaaSize index)
{
    return device_mgr.get_devices().at(index).name.c_str();
}

MaaStringView MaaToolKitGetDeviceAdbPath(MaaSize index)
{
    return device_mgr.get_devices().at(index).adb_path.c_str();
}

MaaStringView MaaToolKitGetDeviceAdbSerial(MaaSize index)
{
    return device_mgr.get_devices().at(index).adb_serial.c_str();
}

MaaAdbControllerType MaaToolKitGetDeviceAdbControllerType(MaaSize index)
{
    return device_mgr.get_devices().at(index).adb_controller_type;
}

MaaStringView MaaToolKitGetDeviceAdbConfig(MaaSize index)
{
    return device_mgr.get_devices().at(index).adb_config.c_str();
}
