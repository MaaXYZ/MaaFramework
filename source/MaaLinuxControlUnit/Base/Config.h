#pragma once
#include "MaaControlUnit/MaaControlUnitDef.h"
#include "UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

struct LinuxControlUnitConfig
{
    MaaLinuxScreencapMethod screencap_method = MaaLinuxScreencapMethod_None;
    MaaLinuxInputMethod input_method = MaaLinuxInputMethod_None;

    std::string wlr_socket_path;

    int pw_socket_fd = -1;
    uint32_t pw_node_id = 0;
    int pw_screen_width = 0;
    int pw_screen_height = 0;

    std::string uinput_path = "/dev/uinput";

    bool use_win32_vk_code = false;

    MEO_JSONIZATION(
        screencap_method, input_method, MEO_OPT wlr_socket_path, MEO_OPT pw_socket_fd, MEO_OPT pw_node_id, MEO_OPT uinput_path,
        MEO_OPT use_win32_vk_code);
};

MAA_CTRL_UNIT_NS_END
