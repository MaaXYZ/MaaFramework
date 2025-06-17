#include "RemoteContext.h"

#include "MaaAgent/Message.hpp"
#include "RemoteTasker.h"
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
        .image = server_.send_image(image),
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
        .box = { box.x, box.y, box.width, box.height },
        .reco_detail = reco_detail,
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

std::optional<json::object> RemoteContext::get_node_data(const std::string& node_name) const
{
    ContextGetNodeDataReverseRequest req {
        .context_id = context_id_,
        .node_name = node_name,
    };

    auto resp_opt = server_.send_and_recv<ContextGetNodeDataReverseResponse>(req);
    if (!resp_opt || !resp_opt->has_value) {
        return std::nullopt;
    }
    return resp_opt->node_data;
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

    auto clone = std::make_unique<RemoteContext>(server_, resp_opt->clone_id);
    auto& ptr = clone_holder_.emplace_back(std::move(clone));

    return ptr.get();
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
    if (tasker_) {
        return tasker_.get();
    }

    ContextTaskerReverseRequest req {
        .context_id = context_id_,
    };

    auto resp_opt = server_.send_and_recv<ContextTaskerReverseResponse>(req);
    if (!resp_opt) {
        return nullptr;
    }

    tasker_ = std::make_unique<RemoteTasker>(server_, resp_opt->tasker_id);
    return tasker_.get();
}

MAA_AGENT_SERVER_NS_END
