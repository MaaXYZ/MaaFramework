#pragma once

#include <functional>

#include "./Context.h"

namespace maapp
{

struct CustomRecognitionRequest
{
    Context context;
    Task task;
    std::string_view node_name;
    std::string_view custom_recognition_name;
    std::string_view custom_recognition_param;
    std::vector<uint8_t> image;
    MaaRect roi;
};

struct CustomRecognitionResponse
{
    bool success;
    MaaRect box;
    std::string detail;
};

struct CustomActionRequest
{
    Context context;
    Task task;
    std::string_view node_name;
    std::string_view custom_action_name;
    std::string_view custom_action_param;
    Reco reco;
    MaaRect box;
};

struct CustomActionResponse
{
    bool success;
};

using CustomRecognition = std::function<CustomRecognitionResponse(CustomRecognitionRequest)>;
using CustomAction = std::function<CustomActionResponse(CustomActionRequest)>;

namespace pri
{

template <typename Tracker, std::tuple<CustomRecognition, Tracker> (*Getter)(void*, const char*)>
MaaBool custom_reco_wrapper(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_recognition_name,
    const char* custom_recognition_param,
    const MaaImageBuffer* image,
    const MaaRect* roi,
    void* trans_arg,
    /* out */ MaaRect* out_box,
    /* out */ MaaStringBuffer* out_detail)
{
    auto [func, tracker] = Getter(trans_arg, custom_recognition_name);
    if (!func) {
        return false;
    }
    auto data = MaaImageBufferGetEncoded(image);
    auto rsp = func(
        CustomRecognitionRequest {
            Context { context },
            Task {
                MaaContextGetTasker(context),
                task_id,
            },
            node_name,
            custom_recognition_name,
            custom_recognition_param,
            std::vector<uint8_t> {
                data,
                data + MaaImageBufferGetEncodedSize(image),
            },
            *roi,
        });
    if (rsp.success) {
        *out_box = rsp.box;
        if (!MaaStringBufferSetEx(out_detail, rsp.detail.c_str(), rsp.detail.length())) {
            throw FunctionFailed("MaaStringBufferSetEx");
        }
        return true;
    }
    else {
        return false;
    }
}

template <typename Tracker, std::tuple<CustomAction, Tracker> (*Getter)(void*, const char*)>
MaaBool custom_act_wrapper(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_action_name,
    const char* custom_action_param,
    MaaRecoId reco_id,
    const MaaRect* box,
    void* trans_arg)
{
    auto [func, tracker] = Getter(trans_arg, custom_action_name);
    if (!func) {
        return false;
    }
    auto rsp = func(
        CustomActionRequest {
            Context { context },
            Task {
                MaaContextGetTasker(context),
                task_id,
            },
            node_name,
            custom_action_name,
            custom_action_param,
            Reco {
                MaaContextGetTasker(context),
                reco_id,
            },
            *box,
        });
    return rsp.success;
}

}

}
