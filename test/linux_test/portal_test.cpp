#include <iostream>

#include "MaaToolkit/MaaToolkitAPI.h"

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
    MaaToolkitPortalHelperDestroy(helper);
}
