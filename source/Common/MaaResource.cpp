#include "MaaFramework/MaaAPI.h"

#include "Common/MaaTypes.h"
#include "Utils/Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MaaBool MaaResourceRegisterCustomRecognition(MaaResource* res, const char* name, MaaCustomRecognitionCallback recognition, void* trans_arg)
{
    LogFunc << VAR_VOIDP(res) << VAR(name) << VAR_VOIDP(recognition) << VAR_VOIDP(trans_arg);

    if (!res || !name || !recognition) {
        LogError << "handle is null";
        return false;
    }

    res->register_custom_recognition(name, recognition, trans_arg);
    return true;
}

MaaBool MaaResourceUnregisterCustomRecognition(MaaResource* res, const char* name)
{
    LogFunc << VAR_VOIDP(res) << VAR(name);

    if (!res || !name) {
        LogError << "handle is null";
        return false;
    }

    res->unregister_custom_recognition(name);
    return true;
}

MaaBool MaaResourceClearCustomRecognition(MaaResource* res)
{
    LogFunc << VAR_VOIDP(res);

    if (!res) {
        LogError << "handle is null";
        return false;
    }

    res->clear_custom_recognition();
    return true;
}

MaaBool MaaResourceRegisterCustomAction(MaaResource* res, const char* name, MaaCustomActionCallback action, void* trans_arg)
{
    LogFunc << VAR_VOIDP(res) << VAR(name) << VAR_VOIDP(action) << VAR_VOIDP(trans_arg);

    if (!res || !name || !action) {
        LogError << "handle is null";
        return false;
    }

    res->register_custom_action(name, action, trans_arg);
    return true;
}

MaaBool MaaResourceUnregisterCustomAction(MaaResource* res, const char* name)
{
    LogFunc << VAR_VOIDP(res) << VAR(name);

    if (!res || !name) {
        LogError << "handle is null";
        return false;
    }

    res->unregister_custom_action(name);
    return true;
}

MaaBool MaaResourceClearCustomAction(MaaResource* res)
{
    LogFunc << VAR_VOIDP(res);

    if (!res) {
        LogError << "handle is null";
        return false;
    }

    res->clear_custom_action();
    return true;
}

MaaResId MaaResourcePostBundle(MaaResource* res, const char* path)
{
    LogFunc << VAR_VOIDP(res) << VAR(path);

    if (!res) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return res->post_bundle(MAA_NS::path(path));
}

MaaBool MaaResourceOverridePipeline(MaaResource* res, const char* pipeline_override)
{
    LogFunc << VAR_VOIDP(res) << VAR(pipeline_override);

    if (!res) {
        LogError << "handle is null";
        return false;
    }
    auto ov_opt = json::parse(pipeline_override);
    if (!ov_opt) {
        LogError << "failed to parse" << VAR(pipeline_override);
        return false;
    }
    if (!ov_opt->is_object()) {
        LogError << "json is not object" << VAR(pipeline_override);
        return false;
    }

    return res->override_pipeline(ov_opt->as_object());
}

MaaBool MaaResourceOverrideNext(MaaResource* res, const char* node_name, const MaaStringListBuffer* next_list)
{
    LogFunc << VAR_VOIDP(res) << VAR(node_name);

    if (!res || !next_list) {
        LogError << "handle is null";
        return false;
    }

    std::vector<std::string> next;

    size_t size = next_list->size();
    for (size_t i = 0; i < size; ++i) {
        next.emplace_back(next_list->at(i).get());
    }

    return res->override_next(node_name, next);
}

MaaBool MaaResourceGetNodeData(MaaResource* res, const char* node_name, MaaStringBuffer* buffer)
{
    LogFunc << VAR_VOIDP(res) << VAR(node_name);

    if (!res || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto data_opt = res->get_node_data(node_name);
    if (!data_opt) {
        LogError << "failed to get node data" << VAR(node_name);
        return false;
    }

    buffer->set(data_opt->dumps());
    return true;
}

MaaBool MaaResourceClear(MaaResource* res)
{
    LogFunc << VAR_VOIDP(res);

    if (!res) {
        LogError << "handle is null";
        return false;
    }

    return res->clear();
}

MaaStatus MaaResourceStatus(const MaaResource* res, MaaResId id)
{
    // LogFunc << VAR_VOIDP(res) << VAR(id);

    if (!res) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return res->status(id);
}

MaaStatus MaaResourceWait(const MaaResource* res, MaaResId id)
{
    // LogFunc << VAR_VOIDP(res) << VAR(id);

    if (!res) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return res->wait(id);
}

MaaBool MaaResourceLoaded(const MaaResource* res)
{
    // LogFunc << VAR_VOIDP(res);

    if (!res) {
        LogError << "handle is null";
        return false;
    }

    return res->valid();
}

MaaBool MaaResourceSetOption(MaaResource* res, MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(res) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!res) {
        LogError << "handle is null";
        return false;
    }

    return res->set_option(key, value, val_size);
}

MaaBool MaaResourceGetHash(const MaaResource* res, MaaStringBuffer* buffer)
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

MaaBool MaaResourceGetNodeList(const MaaResource* res, /* out */ MaaStringListBuffer* buffer)
{
    if (!res || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto list = res->get_node_list();

    buffer->clear();

    for (const auto& name : list) {
        buffer->append(MaaNS::StringBuffer(name));
    }

    return true;
}
