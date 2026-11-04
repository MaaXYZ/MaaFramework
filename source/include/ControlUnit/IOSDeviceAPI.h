#pragma once

#include "MaaControlUnit/MaaControlUnitDef.h"
#include "MaaToolkit/IOSDevice/MaaToolkitIOSDevice.h"

MAA_CTRL_UNIT_NS_BEGIN

class IOSDeviceAPI
{
public:
    virtual ~IOSDeviceAPI() = default;

    virtual MaaBool discover(
        MaaIOSConnectionMethod connection_method,
        const char* options_json,
        MaaToolkitIOSDeviceCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result) = 0;


};

MAA_CTRL_UNIT_NS_END

// Borrowed stateless implementation; its caller must retain the loaded module.
extern "C" MAA_CONTROL_UNIT_API MAA_CTRL_UNIT_NS::IOSDeviceAPI* MaaIOSControlUnitGetDeviceAPI();
