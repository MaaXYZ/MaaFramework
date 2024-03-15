#include "MaaToolkit/Device/MaaToolkitDevice.h"

#include "AdbDevice/DeviceMgrLinux.h"
#include "AdbDevice/DeviceMgrMacOS.h"
#include "AdbDevice/DeviceMgrWin32.h"
#include "Utils/Logger.h"

#if defined(_WIN32)
static MaaToolkitDeviceMgrAPI& device_mgr = MAA_TOOLKIT_NS::DeviceMgrWin32::get_instance();
#elif defined(__linux__)
static MaaToolkitDeviceMgrAPI& device_mgr = MAA_TOOLKIT_NS::DeviceMgrLinux::get_instance();
#elif defined(__APPLE__)
static MaaToolkitDeviceMgrAPI& device_mgr = MAA_TOOLKIT_NS::DeviceMgrMacOS::get_instance();
#endif

MaaSize MaaToolkitFindDevice()
{
    LogWarn << __FUNCTION__
            << "is deprecated, use MaaToolkitPostFindDevice "
               "and MaaToolkitWaitForFindDeviceToComplete instead";

    MaaToolkitPostFindDevice();
    return MaaToolkitWaitForFindDeviceToComplete();
}

MaaSize MaaToolkitFindDeviceWithAdb(MaaStringView adb_path)
{
    LogWarn << __FUNCTION__
            << "is deprecated, use MaaToolkitPostFindDeviceWithAdb "
               "and MaaToolkitWaitForFindDeviceToComplete instead";

    MaaToolkitPostFindDeviceWithAdb(adb_path);
    return MaaToolkitWaitForFindDeviceToComplete();
}

MaaBool MaaToolkitPostFindDevice()
{
    LogFunc;

    return device_mgr.post_find_device();
}

MaaBool MaaToolkitPostFindDeviceWithAdb(MaaStringView adb_path)
{
    LogFunc;

    return device_mgr.post_find_device_with_adb(MAA_NS::path(adb_path));
}

MaaBool MaaToolkitIsFindDeviceCompleted()
{
    return device_mgr.is_find_completed();
}

MaaSize MaaToolkitWaitForFindDeviceToComplete()
{
    while (!MaaToolkitIsFindDeviceCompleted()) {
        std::this_thread::yield();
    }

    return MaaToolkitGetDeviceCount();
}

MaaSize MaaToolkitGetDeviceCount()
{
    auto devices = device_mgr.get_devices();
    if (!devices) {
        return 0;
    }
    return devices->size();
}

MaaStringView MaaToolkitGetDeviceName(MaaSize index)
{
    return device_mgr.get_devices()->at(index).name.c_str();
}

MaaStringView MaaToolkitGetDeviceAdbPath(MaaSize index)
{
    return device_mgr.get_devices()->at(index).adb_path.c_str();
}

MaaStringView MaaToolkitGetDeviceAdbSerial(MaaSize index)
{
    return device_mgr.get_devices()->at(index).adb_serial.c_str();
}

MaaAdbControllerType MaaToolkitGetDeviceAdbControllerType(MaaSize index)
{
    return device_mgr.get_devices()->at(index).adb_controller_type;
}

MaaStringView MaaToolkitGetDeviceAdbConfig(MaaSize index)
{
    return device_mgr.get_devices()->at(index).adb_config.c_str();
}