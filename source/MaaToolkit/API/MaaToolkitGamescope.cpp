#include "MaaToolkit/Gamescope/MaaToolkitGamescope.h"

#include "Gamescope/GamescopeEisSocketBuffer.hpp"
#include "Gamescope/GamescopeEisSocketFinder.h"
#include "Gamescope/GamescopeNodeBuffer.hpp"
#include "Gamescope/GamescopeNodeFinder.h"
#include "MaaUtils/Logger.h"

MaaToolkitGamescopeNodeList* MaaToolkitGamescopeNodeListCreate()
{
    return new MaaToolkitGamescopeNodeList;
}

void MaaToolkitGamescopeNodeListDestroy(MaaToolkitGamescopeNodeList* handle)
{
    if (handle) {
        delete handle;
    }
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
    if (!list) {
        LogError << "list is null";
        return 0;
    }

    return list->size();
}

const MaaToolkitGamescopeNode* MaaToolkitGamescopeNodeListAt(const MaaToolkitGamescopeNodeList* list, MaaSize index)
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

const char* MaaToolkitGamescopeNodeGetName(const MaaToolkitGamescopeNode* node)
{
    if (!node) {
        LogError << "node is null";
        return "";
    }

    return node->name().c_str();
}

uint32_t MaaToolkitGamescopeNodeGetId(const MaaToolkitGamescopeNode* node)
{
    if (!node) {
        LogError << "node is null";
        return 0;
    }

    return node->id();
}

MaaToolkitGamescopeEisSocketList* MaaToolkitGamescopeEisSocketListCreate()
{
    return new MaaToolkitGamescopeEisSocketList;
}

void MaaToolkitGamescopeEisSocketListDestroy(MaaToolkitGamescopeEisSocketList* handle)
{
    if (handle) {
        delete handle;
    }
}

MaaBool MaaToolkitGamescopeEisSocketFindAll(/*out*/ MaaToolkitGamescopeEisSocketList* buffer)
{
    if (!buffer) {
        LogError << "buffer is null";
        return false;
    }

    auto sockets = MAA_TOOLKIT_NS::GamescopeEisSocketFinder::get_instance().find_all();
    for (const auto& socket : sockets) {
        buffer->append(MAA_TOOLKIT_NS::GamescopeEisSocketBuffer(socket.path));
    }

    return true;
}

MaaSize MaaToolkitGamescopeEisSocketListSize(const MaaToolkitGamescopeEisSocketList* list)
{
    if (!list) {
        LogError << "list is null";
        return 0;
    }

    return list->size();
}

const MaaToolkitGamescopeEisSocket* MaaToolkitGamescopeEisSocketListAt(const MaaToolkitGamescopeEisSocketList* list, MaaSize index)
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

const char* MaaToolkitGamescopeEisSocketGetPath(const MaaToolkitGamescopeEisSocket* socket)
{
    if (!socket) {
        LogError << "socket is null";
        return "";
    }

    return socket->path().c_str();
}
