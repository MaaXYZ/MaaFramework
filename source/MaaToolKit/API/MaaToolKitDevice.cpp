#include "MaaToolKit/Device/MaaToolKitDevice.h"

#include "AdbDevice/DeviceMgrLinux.h"
#include "AdbDevice/DeviceMgrMacOS.h"
#include "AdbDevice/DeviceMgrWin32.h"
#include "Utils/Logger.h"

#if defined(_WIN32)
static MaaToolKitDeviceMgrAPI& device_mgr = MAA_TOOLKIT_NS::DeviceMgrWin32::get_instance();
#elif defined(__linux__)
static MaaToolKitDeviceMgrAPI& device_mgr = MAA_TOOLKIT_NS::DeviceMgrLinux::get_instance();
#elif defined(__APPLE__)
static MaaToolKitDeviceMgrAPI& device_mgr = MAA_TOOLKIT_NS::DeviceMgrMacOS::get_instance();
#endif

MaaSize MaaToolKitFindDevice()
{
    LogWarn << "MaaToolKitFindDevice() is deprecated, use MaaToolKitPostFindDevice() and "
               "MaaToolKitWaitForFindDeviceToComplete() instead.";

    MaaToolKitPostFindDevice();
    return MaaToolKitWaitForFindDeviceToComplete();
}

MaaSize MaaToolKitFindDeviceWithAdb(MaaStringView adb_path)
{
    LogWarn << "MaaToolKitFindDeviceWithAdb() is deprecated, use MaaToolKitPostFindDeviceWithAdb() "
               "and MaaToolKitWaitForFindDeviceToComplete() instead.";

    MaaToolKitPostFindDeviceWithAdb(adb_path);
    return MaaToolKitWaitForFindDeviceToComplete();
}

MaaBool MaaToolKitPostFindDevice()
{
    LogFunc;

    return device_mgr.post_find_device();
}

MaaBool MaaToolKitPostFindDeviceWithAdb(MaaStringView adb_path)
{
    LogFunc;

    return device_mgr.post_find_device_with_adb(adb_path);
}

MaaBool MaaToolKitIsFindDeviceCompleted()
{
    return device_mgr.is_find_completed();
}

MaaSize MaaToolKitWaitForFindDeviceToComplete()
{
    while (!MaaToolKitIsFindDeviceCompleted()) {
        std::this_thread::yield();
    }

    return MaaToolKitGetDeviceCount();
}

MaaSize MaaToolKitGetDeviceCount()
{
    return device_mgr.get_devices().size();
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
