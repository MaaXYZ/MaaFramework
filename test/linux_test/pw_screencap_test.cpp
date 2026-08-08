#include <iostream>
#include <thread>

#include "MaaFramework/MaaAPI.h"
#include "MaaToolkit/MaaToolkitAPI.h"

//
// Created by free on 8/8/26.
//
int main()
{
    const auto helper = MaaToolkitPortalHelperCreate();
    std::string restore_token;
    std::cout << "Input Restore Token, press Enter to skip: ";
    getline(std::cin, restore_token);
    if (!restore_token.empty()) {
        MaaToolkitPortalHelperSetRestoreToken(helper, restore_token.c_str());
    }
    MaaToolkitPortalHelperSetPersist(helper, true);
    if (!MaaToolkitPortalHelperOpenStream(helper)) {
        std::cerr << "Failed to open PipeWire Stream" << std::endl;
        return -1;
    }
    auto pw_socket_fd = MaaToolkitPortalHelperGetPipeWireFD(helper);
    auto pw_node_id = MaaToolkitPortalHelperGetPipeWireNodeID(helper);
    auto restore = MaaToolkitPortalHelperGetRestoreToken(helper);
    std::cout << "Socket FD: " << pw_socket_fd << std::endl;
    std::cout << "Node ID: " << pw_node_id << std::endl;
    if (restore != nullptr) {
        std::cout << "Restore Token: " << restore << std::endl;
    }
    auto ctrl = MaaLinuxControllerCreate(
        std::format(
            R"({{"screencap_method":4,"input_method":0, "pw_socket_fd":{}, "pw_node_id":{}, "pw_screen_width":1920, "pw_screen_height":1080}})",
            pw_socket_fd,
            pw_node_id)
            .c_str());
    auto destroy = [&]() {
        MaaControllerDestroy(ctrl);
        MaaToolkitPortalHelperDestroy(helper);
    };
    auto ctrl_id = MaaControllerPostConnection(ctrl);
    MaaControllerWait(ctrl, ctrl_id);
    ctrl_id = MaaControllerPostScreencap(ctrl);
    MaaControllerWait(ctrl, ctrl_id);
    MaaControllerPostInactive(ctrl);
    std::this_thread::sleep_for(std::chrono::seconds(5)); // test inactive
    auto image_buffer = MaaImageBufferCreate();           // Add breakpoint at here to view image
    auto image_result = MaaControllerCachedImage(ctrl, image_buffer);
    if (!image_result) {
        destroy();
        return -1;
    }
    destroy();
    return 0;
}
