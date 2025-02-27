#include "RemoteTasker.h"

#include "MaaAgent/Message.hpp"
#include "Utils/Codec.h"
#include "Utils/Logger.h"

MAA_AGENT_SERVER_NS_BEGIN

RemoteTasker::RemoteTasker(Transceiver& server, const std::string& tasker_id)
    : server_(server)
    , tasker_id_(tasker_id)
{
}

bool RemoteTasker::bind_resource(MaaResource* resource)
{
    LogError << "Can NOT bind resource at remote tasker" << VAR_VOIDP(resource);

    return false;
}

bool RemoteTasker::bind_controller(MaaController* controller)
{
    LogError << "Can NOT bind controller at remote tasker" << VAR_VOIDP(controller);

    return false;
}

bool RemoteTasker::inited() const
{
    TaskerInitedReverseRequest req {
        .tasker_id = tasker_id_,
    };
    auto resp_opt = server_.send_and_recv<TaskerInitedReverseResponse>(req);
    if (!resp_opt) {
        return false;
    }
    return resp_opt->ret;
}

bool RemoteTasker::set_option(MaaTaskerOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogError << "Can NOT set option at remote tasker" << VAR(key) << VAR_VOIDP(value) << VAR(val_size);
    return false;
}

MaaTaskId RemoteTasker::post_task(const std::string& entry, const json::object& pipeline_override)
{
    TaskerPostTaskReverseRequest req {
        .tasker_id = tasker_id_,
        .entry = entry,
        .pipeline_override = pipeline_override,
    };

    auto resp_opt = server_.send_and_recv<TaskerPostTaskReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }

    return resp_opt->task_id;
}

MaaStatus RemoteTasker::status(MaaTaskId task_id) const
{
    TaskerStatusReverseRequest req {
        .tasker_id = tasker_id_,
        .task_id = task_id,
    };

    auto resp_opt = server_.send_and_recv<TaskerStatusReverseResponse>(req);
    if (!resp_opt) {
        return MaaStatus_Invalid;
    }

    return resp_opt->status;
}

MaaStatus RemoteTasker::wait(MaaTaskId task_id) const
{
    TaskerWaitReverseRequest req {
        .tasker_id = tasker_id_,
        .task_id = task_id,
    };

    auto resp_opt = server_.send_and_recv<TaskerWaitReverseResponse>(req);
    if (!resp_opt) {
        return MaaStatus_Invalid;
    }

    return resp_opt->status;
}

bool RemoteTasker::running() const
{
    TaskerRunningReverseRequest req {
        .tasker_id = tasker_id_,
    };

    auto resp_opt = server_.send_and_recv<TaskerRunningReverseResponse>(req);
    if (!resp_opt) {
        return false;
    }

    return resp_opt->ret;
}

MaaTaskId RemoteTasker::post_stop()
{
    TaskerPostStopReverseRequest req {
        .tasker_id = tasker_id_,
    };
    auto resp_opt = server_.send_and_recv<TaskerPostStopReverseResponse>(req);
    if (!resp_opt) {
        return MaaInvalidId;
    }
    return resp_opt->task_id;
}

MaaResource* RemoteTasker::resource() const
{
    TaskerResourceReverseRequest req {
        .tasker_id = tasker_id_,
    };
    auto resp_opt = server_.send_and_recv<TaskerResourceReverseResponse>(req);
    if (!resp_opt) {
        return nullptr;
    }

    resource_ = std::make_unique<RemoteResource>(server_, resp_opt->resource_id);
    return resource_.get();
}

MaaController* RemoteTasker::controller() const
{
    TaskerControllerReverseRequest req {
        .tasker_id = tasker_id_,
    };
    auto resp_opt = server_.send_and_recv<TaskerControllerReverseResponse>(req);
    if (!resp_opt) {
        return nullptr;
    }
    controller_ = std::make_unique<RemoteController>(server_, resp_opt->controller_id);
    return controller_.get();
}

void RemoteTasker::clear_cache()
{
    TaskerClearCacheReverseRequest req {
        .tasker_id = tasker_id_,
    };
    server_.send_and_recv<TaskerClearCacheReverseResponse>(req);
}

std::optional<MAA_TASK_NS::TaskDetail> RemoteTasker::get_task_detail(MaaTaskId task_id) const
{
    TaskerGetTaskDetailReverseRequest req {
        .tasker_id = tasker_id_,
        .task_id = task_id,
    };
    auto resp_opt = server_.send_and_recv<TaskerGetTaskDetailReverseResponse>(req);
    if (!resp_opt) {
        return std::nullopt;
    }

    if (!resp_opt->has_value) {
        return std::nullopt;
    }

    MAA_TASK_NS::TaskDetail result;
    result.task_id = resp_opt->task_id;
    result.entry = resp_opt->entry;
    result.node_ids = resp_opt->node_ids;
    result.status = static_cast<MaaStatus>(resp_opt->status);

    return result;
}

std::optional<MAA_TASK_NS::NodeDetail> RemoteTasker::get_node_detail(MaaNodeId node_id) const
{
    TaskerGetNodeDetailReverseRequest req {
        .tasker_id = tasker_id_,
        .node_id = node_id,
    };

    auto resp_opt = server_.send_and_recv<TaskerGetNodeDetailReverseResponse>(req);
    if (!resp_opt) {
        return std::nullopt;
    }

    if (!resp_opt->has_value) {
        return std::nullopt;
    }

    MAA_TASK_NS::NodeDetail result;
    result.node_id = resp_opt->node_id;
    result.name = resp_opt->name;
    result.reco_id = resp_opt->reco_id;
    result.completed = resp_opt->completed;

    return result;
}

std::optional<MAA_TASK_NS::RecoResult> RemoteTasker::get_reco_result(MaaRecoId reco_id) const
{
    TaskerGetRecoResultReverseRequest req {
        .tasker_id = tasker_id_,
        .reco_id = reco_id,
    };

    auto resp_opt = server_.send_and_recv<TaskerGetRecoResultReverseResponse>(req);
    if (!resp_opt) {
        return std::nullopt;
    }

    if (!resp_opt->has_value) {
        return std::nullopt;
    }

    MAA_TASK_NS::RecoResult result;
    result.reco_id = resp_opt->reco_id;
    result.name = resp_opt->name;
    result.algorithm = resp_opt->algorithm;

    result.box = cv::Rect(resp_opt->box[0], resp_opt->box[1], resp_opt->box[2], resp_opt->box[3]);
    result.detail = resp_opt->detail;
    result.raw = decode_image(resp_opt->raw);
    for (const auto& draw : resp_opt->draws) {
        result.draws.push_back(decode_image(draw));
    }
    return result;
}

std::optional<MaaNodeId> RemoteTasker::get_latest_node(const std::string& node_name) const
{
    TaskerGetLatestNodeReverseRequest req {
        .tasker_id = tasker_id_,
        .node_name = node_name,
    };
    auto resp_opt = server_.send_and_recv<TaskerGetLatestNodeReverseResponse>(req);
    if (!resp_opt) {
        return std::nullopt;
    }
    if (!resp_opt->has_value) {
        return std::nullopt;
    }
    return resp_opt->latest_id;
}

MAA_AGENT_SERVER_NS_END
