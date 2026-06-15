#include "MaaControlUnit/GamescopeControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "Manager/GamescopeControlUnitMgr.h"

const char* MaaGamescopeControlUnitGetVersion()
{
#pragma message("MaaGamescopeControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaGamescopeControlUnitHandle MaaGamescopeControlUnitCreate(uint32_t node_id, const char* eis_socket_path, MaaBool use_win32_vk_code)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR(node_id) << VAR(eis_socket_path) << VAR(use_win32_vk_code);

    if (!eis_socket_path) {
        LogError << "eis_socket_path is null or empty";
        return nullptr;
    }

    auto unit_mgr = std::make_unique<GamescopeControlUnitMgr>(node_id, eis_socket_path, use_win32_vk_code);
    return unit_mgr.release();
}

void MaaGamescopeControlUnitDestroy(MaaGamescopeControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
