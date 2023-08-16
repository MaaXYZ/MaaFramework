#if defined(__linux__)

#include "DeviceMgrLinux.h"

#include <tuple> // for std::ignore

MAA_TOOLKIT_DEVICE_NS_BEGIN

size_t DeviceMgrLinux::find_device(std::string_view adb_path)
{
    // TODO
    std::ignore = adb_path;
    return 0;
}

MAA_TOOLKIT_DEVICE_NS_END

#endif
