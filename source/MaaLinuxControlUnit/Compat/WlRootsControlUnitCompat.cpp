#include "MaaControlUnit/WlRootsControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "Manager/LinuxControlUnitMgr.h"

const char* MaaWlRootsControlUnitGetVersion()
{
    return MAA_VERSION;
}

MaaWlRootsControlUnitHandle MaaWlRootsControlUnitCreate(const char* wlr_socket_path, MaaBool use_win32_vk_code)
{
    LogWarn << "MaaWlRootsControlUnit is deprecated, use MaaLinuxControlUnit instead.";

    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR(wlr_socket_path) << VAR(use_win32_vk_code);

    if (!wlr_socket_path) {
        LogError << "wlr_socket_path is null or empty";
        return nullptr;
    }

    LinuxControlUnitConfig config = { };
    config.screencap_method = MaaLinuxScreencapMethod_Wlr;
    config.input_method = MaaLinuxInputMethod_Wlr;
    config.wlr_socket_path = wlr_socket_path;
    config.use_win32_vk_code = use_win32_vk_code;

    auto unit_mgr = std::make_unique<LinuxControlUnitMgr>(config);
    return unit_mgr.release();
}

void MaaWlRootsControlUnitDestroy(MaaWlRootsControlUnitHandle handle)
{
    LogWarn << "MaaWlRootsControlUnit is deprecated, use MaaLinuxControlUnit instead.";

    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
