#include "MaaToolkit/AdbDevice/MaaToolkitAdbDevice.h"

#include "AdbDevice/AdbDeviceBuffer.hpp"
#include "AdbDevice/AdbDeviceLinuxFinder.h"
#include "AdbDevice/AdbDeviceMacOSFinder.h"
#include "AdbDevice/AdbDeviceWin32Finder.h"
#include "MaaUtils/Logger.h"

static MAA_TOOLKIT_NS::AdbDeviceFinder& finder()
{
    using namespace MAA_TOOLKIT_NS;

#if defined(_WIN32)
    return AdbDeviceWin32Finder::get_instance();
#elif defined(__APPLE__)
    return AdbDeviceMacOSFinder::get_instance();
#elif defined(__linux__)
    return AdbDeviceLinuxFinder::get_instance();
#endif
}

MaaToolkitAdbDeviceList* MaaToolkitAdbDeviceListCreate()
{
    return new MaaToolkitAdbDeviceList;
}

void MaaToolkitAdbDeviceListDestroy(MaaToolkitAdbDeviceList* handle)
{
    if (handle) {
        delete handle;
    }
}

MaaBool MaaToolkitAdbDeviceFind(MaaToolkitAdbDeviceList* buffer)
{
    if (!buffer) {
        LogError << "buffer is null";
        return false;
    }

    auto devices = finder().find();
    for (const auto& d : devices) {
        buffer->append(MAA_TOOLKIT_NS::AdbDeviceBuffer(d));
    }

    return true;
}

MaaBool MaaToolkitAdbDeviceFindSpecified(const char* adb_path, MaaToolkitAdbDeviceList* buffer)
{
    if (!buffer) {
        LogError << "buffer is null";
        return false;
    }

    if (!adb_path) {
        LogError << "adb_path is null";
        return false;
    }

    auto devices = finder().find_specified(MAA_NS::path(adb_path));
    for (const auto& d : devices) {
        buffer->append(MAA_TOOLKIT_NS::AdbDeviceBuffer(d));
    }

    return true;
}

MaaSize MaaToolkitAdbDeviceListSize(const MaaToolkitAdbDeviceList* list)
{
    if (!list) {
        LogError << "buffer is null";
        return 0;
    }

    return list->size();
}

const MaaToolkitAdbDevice* MaaToolkitAdbDeviceListAt(const MaaToolkitAdbDeviceList* list, MaaSize index)
{
    if (!list) {
        LogError << "buffer is null";
        return nullptr;
    }
    if (index >= list->size()) {
        LogError << "out of range" << VAR(index) << VAR(list->size());
        return nullptr;
    }

    return &list->at(index);
}

static const char* kEmptyCStr = "";

const char* MaaToolkitAdbDeviceGetName(const MaaToolkitAdbDevice* device)
{
    if (!device) {
        LogError << "device is null";
        return kEmptyCStr;
    }

    return device->name().c_str();
}

const char* MaaToolkitAdbDeviceGetAdbPath(const MaaToolkitAdbDevice* device)
{
    if (!device) {
        LogError << "device is null";
        return kEmptyCStr;
    }

    return device->adb_path().c_str();
}

const char* MaaToolkitAdbDeviceGetAddress(const MaaToolkitAdbDevice* device)
{
    if (!device) {
        LogError << "device is null";
        return kEmptyCStr;
    }

    return device->address().c_str();
}

MaaAdbScreencapMethod MaaToolkitAdbDeviceGetScreencapMethods(const MaaToolkitAdbDevice* device)
{
    if (!device) {
        LogError << "device is null";
        return MaaAdbScreencapMethod_None;
    }

    return device->screencap_methods();
}

MaaAdbInputMethod MaaToolkitAdbDeviceGetInputMethods(const MaaToolkitAdbDevice* device)
{
    if (!device) {
        LogError << "device is null";
        return MaaAdbInputMethod_None;
    }

    return device->input_methods();
}

const char* MaaToolkitAdbDeviceGetConfig(const MaaToolkitAdbDevice* device)
{
    if (!device) {
        LogError << "device is null";
        return kEmptyCStr;
    }

    return device->config().c_str();
}
