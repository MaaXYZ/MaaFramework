#pragma once

#include <memory>
#include <vector>

#include "Common/MaaTypes.h"
#include "MaaAgent/Transceiver.h"
#include "RemoteTasker.h"

MAA_AGENT_SERVER_NS_BEGIN

class RemoteContext : public MaaContext
{
public:
    RemoteContext(Transceiver& server, const std::string& context_id);
    virtual ~RemoteContext() = default;

    virtual MaaTaskId run_task(const std::string& entry, const json::object& pipeline_override) override;
    virtual MaaRecoId run_recognition(const std::string& entry, const json::object& pipeline_override, const cv::Mat& image) override;
    virtual MaaNodeId
        run_action(const std::string& entry, const json::object& pipeline_override, const cv::Rect& box, const std::string& reco_detail)
            override;
    virtual bool override_pipeline(const json::object& pipeline_override) override;
    virtual bool override_next(const std::string& node_name, const std::vector<std::string>& next) override;
    virtual std::optional<json::object> get_node_data(const std::string& node_name) const override;

    virtual MaaContext* clone() const override;

    virtual MaaTaskId task_id() const override;
    virtual MaaTasker* tasker() const override;

private:
    Transceiver& server_;
    std::string context_id_;

    mutable std::vector<std::unique_ptr<RemoteContext>> clone_holder_;
    mutable std::unique_ptr<RemoteTasker> tasker_ = nullptr;
};

MAA_AGENT_SERVER_NS_END
