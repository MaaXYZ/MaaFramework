#include "RemoteContext.h"

#include "MaaAgent/Message.hpp"
#include "MaaUtils/Encoding.h"
#include "RemoteTasker.h"

MAA_AGENT_SERVER_NS_BEGIN

RemoteContext::RemoteContext(Transceiver& server, const std::string& context_id)
    : server_(server)
    , context_id_(context_id)
{
}

MaaTaskId RemoteContext::run_task(const std::string& entry, const json::value& pipeline_override)
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

MaaRecoId RemoteContext::run_recognition(const std::string& entry, const json::value& pipeline_override, const cv::Mat& image)
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

MaaActId RemoteContext::run_action(
    const std::string& entry,
    const json::value& pipeline_override,
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
    return resp_opt->action_id;
}

bool RemoteContext::override_pipeline(const json::value& pipeline_override)
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

bool RemoteContext::override_image(const std::string& image_name, const cv::Mat& image)
{
    ContextOverrideImageReverseRequest req {
        .context_id = context_id_,
        .image_name = image_name,
        .image = server_.send_image(image),
    };

    auto resp_opt = server_.send_and_recv<ContextOverrideImageReverseResponse>(req);
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

void RemoteContext::set_anchor(const std::string& anchor_name, const std::string& node_name)
{
    ContextSetAnchorReverseRequest req {
        .context_id = context_id_,
        .anchor_name = anchor_name,
        .node_name = node_name,
    };

    server_.send_and_recv<ContextSetAnchorReverseResponse>(req);
}

std::optional<std::string> RemoteContext::get_anchor(const std::string& anchor_name) const
{
    ContextGetAnchorReverseRequest req {
        .context_id = context_id_,
        .anchor_name = anchor_name,
    };

    auto resp_opt = server_.send_and_recv<ContextGetAnchorReverseResponse>(req);
    if (!resp_opt || !resp_opt->has_value) {
        return std::nullopt;
    }
    return resp_opt->node_name;
}

size_t RemoteContext::get_hit_count(const std::string& node_name) const
{
    ContextGetHitCountReverseRequest req {
        .context_id = context_id_,
        .node_name = node_name,
    };

    auto resp_opt = server_.send_and_recv<ContextGetHitCountReverseResponse>(req);
    if (!resp_opt) {
        return 0;
    }
    return resp_opt->count;
}

void RemoteContext::clear_hit_count(const std::string& node_name)
{
    ContextClearHitCountReverseRequest req {
        .context_id = context_id_,
        .node_name = node_name,
    };

    server_.send_and_recv<ContextClearHitCountReverseResponse>(req);
}

MAA_AGENT_SERVER_NS_END
