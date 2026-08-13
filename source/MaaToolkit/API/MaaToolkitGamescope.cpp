#include "MaaToolkit/Gamescope/MaaToolkitGamescope.h"

#include "Gamescope/GamescopeNodeBuffer.hpp"
#include "Gamescope/GamescopeNodeFinder.h"
#include "MaaUtils/Logger.h"

MaaToolkitGamescopeNodeList* MaaToolkitGamescopeNodeListCreate()
{
    return new MaaToolkitGamescopeNodeList;
}

void MaaToolkitGamescopeNodeListDestroy(MaaToolkitGamescopeNodeList* handle)
{
    delete handle;
}

MaaBool MaaToolkitGamescopeNodeFindAll(/*out*/ MaaToolkitGamescopeNodeList* buffer)
{
    if (!buffer) {
        LogError << "buffer is null";
        return false;
    }

    auto nodes = MAA_TOOLKIT_NS::GamescopeNodeFinder::get_instance().find_all();
    for (const auto& node : nodes) {
        buffer->append(MAA_TOOLKIT_NS::GamescopeNodeBuffer(node.id, node.name));
    }
    return true;
}

MaaSize MaaToolkitGamescopeNodeListSize(const MaaToolkitGamescopeNodeList* list)
{
    return list ? list->size() : 0;
}

const MaaToolkitGamescopeNode* MaaToolkitGamescopeNodeListAt(const MaaToolkitGamescopeNodeList* list, MaaSize index)
{
    if (!list || index >= list->size()) {
        return nullptr;
    }
    return &list->at(index);
}

const char* MaaToolkitGamescopeNodeGetName(const MaaToolkitGamescopeNode* node)
{
    return node ? node->name().c_str() : nullptr;
}

uint32_t MaaToolkitGamescopeNodeGetId(const MaaToolkitGamescopeNode* node)
{
    return node ? node->id() : 0;
}
