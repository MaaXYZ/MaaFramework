#include "RemoteContext.h"

#include "MaaAgent/Message.hpp"
#include "Utils/Codec.h"

MAA_AGENT_SERVER_NS_BEGIN

RemoteContext::RemoteContext(Transceiver& server, const std::string& context_id)
    : server_(server)
    , context_id_(context_id)
{
}

MaaTaskId RemoteContext::run_task(const std::string& entry, const json::object& pipeline_override)
{
    ContextRunTaskReverseRequest req {
        .context_id = context_id_,
        .entry = entry,
        .pipeline_override = pipeline_override,
    };

    auto resp_opt = server_.send_and_recv<ContextRunTaskReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->task_id;
}

MaaRecoId RemoteContext::run_recognition(const std::string& entry, const json::object& pipeline_override, const cv::Mat& image)
{
    ContextRunRecognitionReverseRequest req {
        .context_id = context_id_,
        .entry = entry,
        .pipeline_override = pipeline_override,
        .image = encode_image(image)
    };
    
    auto resp_opt = server_.send_and_recv<ContextRunRecognitionReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->reco_id;
}

MaaNodeId RemoteContext::run_action(
    const std::string& entry,
    const json::object& pipeline_override,
    const cv::Rect& box,
    const std::string& reco_detail)
{
    ContextRunActionReverseRequest req {
        .context_id = context_id_,
        .entry = entry,
        .pipeline_override = pipeline_override,
        .box = {box.x, box.y, box.width, box.height},
        .reco_detail = reco_detail
    };

    auto resp_opt = server_.send_and_recv<ContextRunActionReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->node_id;
}

bool RemoteContext::override_pipeline(const json::object& pipeline_override)
{
    ContextOverridePipelineReverseRequest req {
        .context_id = context_id_,
        .pipeline_override = pipeline_override,
    };

    auto resp_opt = server_.send_and_recv<ContextOverridePipelineReverseResponse>(req);
    if (!resp_opt) {
        return false;
    }
    return resp_opt->ret;
}

bool RemoteContext::override_next(const std::string& node_name, const std::vector<std::string>& next)
{
    ContextOverrideNextReverseRequest req {
        .context_id = context_id_,
        .node_name = node_name,
        .next = next,
    };

    auto resp_opt = server_.send_and_recv<ContextOverrideNextReverseResponse>(req);
    if (!resp_opt) {
        return false;
    }
    return resp_opt->ret;
}

MaaContext* RemoteContext::clone() const
{
    ContextCloneReverseRequest req {
        .context_id = context_id_,
    };
    
    auto resp_opt = server_.send_and_recv<ContextCloneReverseResponse>(req);
    if (!resp_opt) {
        return nullptr;
    }
    //TODO
    //std::string cloned_id = resp_opt->cloned_context_id;

    return nullptr;
}

MaaTaskId RemoteContext::task_id() const
{
    ContextTaskIdReverseRequest req {
        .context_id = context_id_,
    };
    
    auto resp_opt = server_.send_and_recv<ContextTaskIdReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }

    return resp_opt->task_id;
}

MaaTasker* RemoteContext::tasker() const
{    
    ContextTaskerReverseRequest req {
        .context_id = context_id_,
    };
    
    auto resp_opt = server_.send_and_recv<ContextTaskerReverseResponse>(req);
    if (!resp_opt) {
        return nullptr;
    }
    //TODO
    //std::string tasker_id = resp_opt->tasker_id;

    return nullptr;
}

MAA_AGENT_SERVER_NS_END
