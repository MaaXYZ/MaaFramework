#pragma once

#include <memory>

#include "Common/MaaTypes.h"
#include "MaaAgent/Transceiver.h"
#include "RemoteController.h"
#include "RemoteResource.h"

MAA_AGENT_SERVER_NS_BEGIN

class RemoteTasker : public MaaTasker
{
public:
    RemoteTasker(Transceiver& server, const std::string& tasker_id);
    virtual ~RemoteTasker() = default;

    virtual bool bind_resource(MaaResource* resource) override;
    virtual bool bind_controller(MaaController* controller) override;
    virtual bool inited() const override;

    virtual bool set_option(MaaTaskerOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaTaskId post_task(const std::string& entry, const json::object& pipeline_override) override;

    virtual MaaStatus status(MaaTaskId task_id) const override;
    virtual MaaStatus wait(MaaTaskId task_id) const override;

    virtual bool running() const override;
    virtual MaaTaskId post_stop() override;

    virtual MaaResource* resource() const override;
    virtual MaaController* controller() const override;

    virtual void clear_cache() override;
    virtual std::optional<MAA_TASK_NS::TaskDetail> get_task_detail(MaaTaskId task_id) const override;
    virtual std::optional<MAA_TASK_NS::NodeDetail> get_node_detail(MaaNodeId node_id) const override;
    virtual std::optional<MAA_TASK_NS::RecoResult> get_reco_result(MaaRecoId reco_id) const override;
    virtual std::optional<MaaNodeId> get_latest_node(const std::string& node_name) const override;

private:
    Transceiver& server_;
    std::string tasker_id_;

    mutable std::unique_ptr<RemoteResource> resource_ = nullptr;
    mutable std::unique_ptr<RemoteController> controller_ = nullptr;
};

MAA_AGENT_SERVER_NS_END
