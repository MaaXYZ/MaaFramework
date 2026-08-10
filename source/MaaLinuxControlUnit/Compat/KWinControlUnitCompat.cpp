#include "Base/Config.h"
#include "Manager/LinuxControlUnitMgr.h"

#include "MaaControlUnit/KWinControlUnitAPI.h"
#include "MaaToolkit/Portal/MaaToolkitPortal.h"

#include "MaaUtils/Logger.h"

const char* MaaKWinControlUnitGetVersion()
{
    return MAA_VERSION;
}

MaaKWinControlUnitHandle MaaKWinControlUnitCreate(const char* device_node, int screen_width, int screen_height, MaaBool use_win32_vk_code)
{
    LogWarn << "MaaKWinControlUnit is deprecated, use MaaLinuxControlUnit instead.";

    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR(device_node) << VAR(screen_width) << VAR(screen_height);

    if (!device_node || device_node[0] == '\0') {
        LogError << "device_node is null or empty";
        return nullptr;
    }

    if (screen_width <= 0 || screen_height <= 0) {
        LogError << "Invalid screen dimensions" << VAR(screen_width) << VAR(screen_height) << VAR(use_win32_vk_code);
        return nullptr;
    }

    LinuxControlUnitConfig config = { };
    config.screencap_method = MaaLinuxScreencapMethod_PipeWire;
    config.input_method = MaaLinuxInputMethod_UInput;
    config.pw_screen_width = screen_width;
    config.pw_screen_height = screen_height;
    config.uinput_path = device_node;
    config.use_win32_vk_code = use_win32_vk_code;

    const auto helper = MaaToolkitPortalHelperCreate();
    if (!MaaToolkitPortalHelperOpenStream(helper)) {
        LogError << "Failed to open PipeWire Stream";
        return nullptr;
    }
    config.pw_socket_fd = MaaToolkitPortalHelperGetPipeWireFD(helper);
    config.pw_node_id = MaaToolkitPortalHelperGetPipeWireNodeID(helper);
    MaaToolkitPortalHelperDestroy(helper);

    auto unit_mgr = std::make_unique<LinuxControlUnitMgr>(config);
    return unit_mgr.release();
}

void MaaKWinControlUnitDestroy(MaaKWinControlUnitHandle handle)
{
    LogWarn << "MaaKWinControlUnit is deprecated, use MaaLinuxControlUnit instead.";

    LogFunc << VAR_VOIDP(handle);

    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}
