#include <iostream>

#include "MaaToolkit/MaaToolkitAPI.h"

int main()
{
    const auto helper = MaaToolkitPortalHelperCreate();
    if (!MaaToolkitPortalHelperOpenStream(helper)) {
        std::cerr << "Failed to open PipeWire Stream" << std::endl;
        return -1;
    }
    auto pw_socket_fd = MaaToolkitPortalHelperGetPipeWireFD(helper);
    auto pw_node_id = MaaToolkitPortalHelperGetPipeWireNodeID(helper);
    std::cout << "Socket FD: " << pw_socket_fd << std::endl;
    std::cout << "Node ID: " << pw_node_id << std::endl;
    MaaToolkitPortalHelperDestroy(helper);
}
