#include "MaaToolkit/Gamescope/MaaToolkitGamescope.h"

#include "Gamescope/GamescopeFinder.h"
#include "Gamescope/GamescopeInstanceBuffer.hpp"
#include "MaaUtils/Logger.h"

MaaToolkitGamescopeInstanceList* MaaToolkitGamescopeInstanceListCreate()
{
    return new MaaToolkitGamescopeInstanceList;
}

void MaaToolkitGamescopeInstanceListDestroy(MaaToolkitGamescopeInstanceList* handle)
{
    if (handle) {
        delete handle;
    }
}

MaaBool MaaToolkitGamescopeInstanceFindAll(/*out*/ MaaToolkitGamescopeInstanceList* buffer)
{
    if (!buffer) {
        LogError << "buffer is null";
        return false;
    }

    auto instances = MAA_TOOLKIT_NS::GamescopeFinder::get_instance().find_all();
    for (auto& instance : instances) {
        buffer->append(MAA_TOOLKIT_NS::GamescopeInstanceBuffer(std::move(instance)));
    }

    return true;
}

MaaSize MaaToolkitGamescopeInstanceListSize(const MaaToolkitGamescopeInstanceList* list)
{
    if (!list) {
        LogError << "list is null";
        return 0;
    }

    return list->size();
}

const MaaToolkitGamescopeInstance* MaaToolkitGamescopeInstanceListAt(const MaaToolkitGamescopeInstanceList* list, MaaSize index)
{
    if (!list) {
        LogError << "list is null";
        return nullptr;
    }
    if (index >= list->size()) {
        LogError << "out of range" << VAR(index) << VAR(list->size());
        return nullptr;
    }

    return &list->at(index);
}

uint32_t MaaToolkitGamescopeInstanceGetDisplayNo(const MaaToolkitGamescopeInstance* instance)
{
    if (!instance) {
        LogError << "instance is null";
        return 0;
    }

    return instance->display_no();
}

uint32_t MaaToolkitGamescopeInstanceGetPipeWireNodeId(const MaaToolkitGamescopeInstance* instance)
{
    if (!instance) {
        LogError << "instance is null";
        return 0;
    }

    return instance->pipewire_node_id();
}

const char* MaaToolkitGamescopeInstanceGetEisSocketPath(const MaaToolkitGamescopeInstance* instance)
{
    if (!instance) {
        LogError << "instance is null";
        return "";
    }

    return instance->eis_socket_path().c_str();
}
