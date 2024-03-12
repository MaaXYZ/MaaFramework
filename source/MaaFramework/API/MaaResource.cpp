#include "MaaFramework/MaaAPI.h"

#include <meojson/json.hpp>

#include "Buffer/StringBuffer.hpp"
#include "Resource/ResourceMgr.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MaaResourceHandle
    MaaResourceCreate(MaaResourceCallback callback, MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    return new MAA_RES_NS::ResourceMgr(callback, callback_arg);
}

void MaaResourceDestroy(MaaResourceHandle res)
{
    LogFunc << VAR_VOIDP(res);

    if (res == nullptr) {
        LogError << "handle is null";
        return;
    }

    delete res;
}

MaaResId MaaResourcePostPath(MaaResourceHandle res, MaaStringView path)
{
    LogFunc << VAR_VOIDP(res) << VAR(path);

    if (!res) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return res->post_path(MAA_NS::path(path));
}

MaaBool MaaResourceClear(MaaResourceHandle res)
{
    LogFunc << VAR_VOIDP(res);

    if (!res) {
        LogError << "handle is null";
        return false;
    }

    return res->clear();
}

MaaStatus MaaResourceStatus(MaaResourceHandle res, MaaResId id)
{
    // LogFunc << VAR_VOIDP(res) << VAR(id);

    if (!res) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return res->status(id);
}

MaaStatus MaaResourceWait(MaaResourceHandle res, MaaResId id)
{
    // LogFunc << VAR_VOIDP(res) << VAR(id);

    if (!res) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return res->wait(id);
}

MaaBool MaaResourceLoaded(MaaResourceHandle res)
{
    // LogFunc << VAR_VOIDP(res);

    if (!res) {
        LogError << "handle is null";
        return false;
    }

    return res->valid();
}

MaaBool MaaResourceSetOption(
    MaaResourceHandle res,
    MaaResOption key,
    MaaOptionValue value,
    MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(res) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!res) {
        LogError << "handle is null";
        return false;
    }

    return res->set_option(key, value, val_size);
}

MaaBool MaaResourceGetHash(MaaResourceHandle res, MaaStringBufferHandle buffer)
{
    if (!res || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto hash = res->get_hash();
    if (hash.empty()) {
        LogError << "hash is empty";
        return false;
    }

    buffer->set(std::move(hash));
    return true;
}

MaaBool MaaResourceGetTaskList(MaaResourceHandle res, /* out */ MaaStringBufferHandle buffer)
{
    if (!res || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto list = res->get_task_list();
    if (list.empty()) {
        LogError << "list is empty";
        return false;
    }

    buffer->set(json::array(list).to_string());
    return true;
}