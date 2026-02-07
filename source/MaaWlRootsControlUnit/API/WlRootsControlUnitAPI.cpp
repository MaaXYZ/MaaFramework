#include "ControlUnit/WlRootsControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "Manager/WlRootsControlUnitMgr.h"

const char* MaaWlRootsControlUnitGetVersion()
{
#pragma message("MaaWlRootsControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaControlUnitHandle MaaWlRootsControlUnitCreate(const char* wlr_socket_path)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR(wlr_socket_path);

    if (!wlr_socket_path) {
        LogError << "wlr_socket_path is null or empty";
        return nullptr;
    }

    auto unit_mgr = std::make_unique<WlRootsControlUnitMgr>(wlr_socket_path);
    return unit_mgr.release();
}

void MaaWlRootsControlUnitDestroy(MaaControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
