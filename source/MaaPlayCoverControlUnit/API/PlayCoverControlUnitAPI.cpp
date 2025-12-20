#include "ControlUnit/PlayCoverControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "Manager/PlayCoverControlUnitMgr.h"

const char* MaaPlayCoverControlUnitGetVersion()
{
#pragma message("MaaPlayCoverControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaControlUnitHandle MaaPlayCoverControlUnitCreate(const char* address, const char* uuid)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR(address) << VAR(uuid);

    if (!address || !uuid || !address[0] || !uuid[0]) {
        LogError << "address or uuid is null or empty";
        return nullptr;
    }

    auto unit_mgr = std::make_unique<PlayCoverControlUnitMgr>(address, uuid);
    return unit_mgr.release();
}

void MaaPlayCoverControlUnitDestroy(MaaControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
