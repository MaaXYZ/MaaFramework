#include "ControlUnit/ThriftControlUnitAPI.h"

#include <meojson/json.hpp>

#include "Utils/Logger.h"
#include "impl/ThriftControllerSocketAgent.h"
#include "impl/ThriftControllerUnixDomainSocketAgent.h"

using namespace MAA_CTRL_UNIT_NS;

MaaStringView MaaThriftControlUnitGetVersion()
{
#pragma message("MaaThriftControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaControlUnitHandle MAA_CONTROL_UNIT_API MaaThriftControlUnitCreate(
    MaaThriftControllerType type,
    MaaStringView host,
    int32_t port,
    MaaStringView config)
{
    LogFunc << VAR(type) << VAR(host) << VAR(port) << VAR(config);

    MaaControlUnitHandle handle = nullptr;

    switch (type) {
    case MaaThriftControllerType_Socket:
        handle = new ThriftControllerSocketAgent(host, port);
        break;

    case MaaThriftControllerType_UnixDomainSocket:
        handle = new ThriftControllerUnixDomainSocketAgent(host);
        break;
    }

    LogTrace << VAR_VOIDP(handle);

    return handle;
}

void MaaThriftControlUnitDestroy(MaaControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}