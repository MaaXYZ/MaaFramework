#pragma once

#include "Common/MaaTypes.h"

MAA_AGENT_SERVER_NS_BEGIN

class RemoteContext : public MaaContext
{
public:
    RemoteContext(const std::string& context_id);
    virtual ~RemoteContext() = default;

    virtual MaaTaskId run_task(const std::string& entry, const json::object& pipeline_override) override;
    virtual MaaRecoId run_recognition(const std::string& entry, const json::object& pipeline_override, const cv::Mat& image) override;
    virtual MaaNodeId
        run_action(const std::string& entry, const json::object& pipeline_override, const cv::Rect& box, const std::string& reco_detail)
            override;
    virtual bool override_pipeline(const json::object& pipeline_override) override;
    virtual bool override_next(const std::string& node_name, const std::vector<std::string>& next) override;

    virtual MaaContext* clone() const override;

    virtual MaaTaskId task_id() const override;
    virtual MaaTasker* tasker() const override;

private:
    std::string context_id_ = 0;
};

MAA_AGENT_SERVER_NS_END
