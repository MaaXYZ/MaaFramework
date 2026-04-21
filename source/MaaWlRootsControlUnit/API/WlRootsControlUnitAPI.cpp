#include "MaaControlUnit/WlRootsControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "Manager/WlRootsControlUnitMgr.h"

const char* MaaWlRootsControlUnitGetVersion()
{
#pragma message("MaaWlRootsControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaWlRootsControlUnitHandle MaaWlRootsControlUnitCreate(const char* wlr_socket_path, MaaBool use_win32_vk_code)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR(wlr_socket_path) << VAR(use_win32_vk_code);

    if (!wlr_socket_path) {
        LogError << "wlr_socket_path is null or empty";
        return nullptr;
    }

    auto unit_mgr = std::make_unique<WlRootsControlUnitMgr>(wlr_socket_path, use_win32_vk_code);
    return unit_mgr.release();
}

void MaaWlRootsControlUnitDestroy(MaaWlRootsControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
