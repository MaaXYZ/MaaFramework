#include "MaaFramework/MaaAPI.h"

#include "Common/MaaTypes.h"
#include "MaaUtils/Buffer/StringBuffer.hpp"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MaaSinkId MaaResourceAddSink(MaaResource* res, MaaEventCallback sink, void* trans_arg)
{
    LogInfo << VAR_VOIDP(res) << VAR_VOIDP(sink) << VAR_VOIDP(trans_arg);

    if (!res) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return res->add_sink(sink, trans_arg);
}

void MaaResourceRemoveSink(MaaResource* res, MaaSinkId sink_id)
{
    LogInfo << VAR_VOIDP(res) << VAR(sink_id);

    if (!res) {
        LogError << "handle is null";
        return;
    }

    res->remove_sink(sink_id);
}

void MaaResourceClearSinks(MaaResource* res)
{
    LogInfo << VAR_VOIDP(res);

    if (!res) {
        LogError << "handle is null";
        return;
    }

    res->clear_sinks();
}

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

    if (!path) {
        LogError << "path is null";
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

    if (!pipeline_override) {
        LogError << "pipeline_override is null";
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

    if (!node_name) {
        LogError << "node_name is null";
        return false;
    }

    std::vector<std::string> next;

    size_t size = next_list->size();
    for (size_t i = 0; i < size; ++i) {
        next.emplace_back(next_list->at(i).get());
    }

    return res->override_next(node_name, next);
}

MaaBool MaaResourceOverrideImage(MaaResource* res, const char* image_name, const MaaImageBuffer* image)
{
    LogFunc << VAR_VOIDP(res) << VAR(image_name);

    if (!res || !image) {
        LogError << "handle is null";
        return false;
    }

    if (!image_name) {
        LogError << "image_name is null";
        return false;
    }

    const cv::Mat& mat = image->get();

    return res->override_image(image_name, mat);
}

MaaBool MaaResourceGetNodeData(MaaResource* res, const char* node_name, MaaStringBuffer* buffer)
{
    LogFunc << VAR_VOIDP(res) << VAR(node_name);

    if (!res || !buffer) {
        LogError << "handle is null";
        return false;
    }

    if (!node_name) {
        LogError << "node_name is null";
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

MaaBool MaaResourceGetCustomRecognitionList(const MaaResource* res, /* out */ MaaStringListBuffer* buffer)
{
    if (!res || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto list = res->get_custom_recognition_list();

    buffer->clear();

    for (const auto& name : list) {
        buffer->append(MaaNS::StringBuffer(name));
    }

    return true;
}

MaaBool MaaResourceGetCustomActionList(const MaaResource* res, /* out */ MaaStringListBuffer* buffer)
{
    if (!res || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto list = res->get_custom_action_list();

    buffer->clear();

    for (const auto& name : list) {
        buffer->append(MaaNS::StringBuffer(name));
    }

    return true;
}

MaaRecoId MaaResourcePostRecognition(MaaResource* res, const MaaImageBuffer* image, const char* type, const char* param)
{
    LogFunc << VAR_VOIDP(res) << VAR(type) << VAR(param);

    if (!res || !image || !type || !param) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    auto param_opt = json::parse(param);
    if (!param_opt || !param_opt->is_object()) {
        LogError << "failed to parse param" << VAR(param);
        return MaaInvalidId;
    }

    return res->post_recognition(image->get(), type, *param_opt);
}

MaaStatus MaaResourceRecoStatus(const MaaResource* res, MaaRecoId reco_id)
{
    if (!res) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return res->reco_status(reco_id);
}

MaaStatus MaaResourceRecoWait(const MaaResource* res, MaaRecoId reco_id)
{
    if (!res) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return res->reco_wait(reco_id);
}

MaaBool MaaResourceGetRecoDetail(
    const MaaResource* res,
    MaaRecoId reco_id,
    /* out */ MaaStringBuffer* name,
    /* out */ MaaBool* hit,
    /* out */ MaaRect* hit_box,
    /* out */ MaaStringBuffer* detail_json)
{
    if (!res) {
        LogError << "handle is null";
        return false;
    }

    auto result_opt = res->get_reco_result(reco_id);
    if (!result_opt) {
        LogError << "failed to get reco result" << VAR(reco_id);
        return false;
    }

    const auto& result = *result_opt;

    if (name) {
        name->set(result.name);
    }

    if (hit) {
        *hit = result.box.has_value();
    }

    if (hit_box && result.box) {
        hit_box->x = result.box->x;
        hit_box->y = result.box->y;
        hit_box->width = result.box->width;
        hit_box->height = result.box->height;
    }

    if (detail_json) {
        detail_json->set(result.detail.dumps());
    }

    return true;
}
