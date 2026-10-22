#include "MaaToolkit/IOSDevice/MaaToolkitIOSDevice.h"

#include "ControlUnit/IOSDeviceAPI.h"
#include "LibraryHolder/ControlUnit.h"
#include "MaaFramework/Utility/MaaBuffer.h"
#include "MaaUtils/ScopeLeave.hpp"

namespace
{
template <typename Function>
MaaBool invoke(MaaStringBuffer* result, Function action)
{
    if (!result) {
        return false;
    }
    try {
        auto api = MAA_NS::IOSControlUnitLibraryHolder::device_api();
        if (api) {
            return action(api);
        }
        MaaStringBufferSet(result, R"({"success":false,"cancelled":false,"error":"Failed to load iOS device management"})");
    }
    catch (...) {
        MaaStringBufferSet(result, R"({"success":false,"cancelled":false,"error":"iOS device management failed"})");
    }
    return false;
}
} // namespace

MaaBool MaaToolkitIOSDeviceDiscover(
    MaaIOSConnectionMethod connection_method,
    const char* options,
    MaaToolkitIOSDeviceEventCallback event,
    MaaToolkitIOSDeviceCancelCallback cancel,
    void* userdata,
    MaaStringBuffer* result)
{
    return invoke(result, [&](const auto& api) { return api->discover(connection_method, options, event, cancel, userdata, result); });
}
