#include "RemoteContext.h"

#include "MaaAgent/Message.hpp"

MAA_AGENT_SERVER_NS_BEGIN

RemoteContext::RemoteContext(const std::string& context_id)
    : context_id_(context_id)
{
}

MaaTaskId RemoteContext::run_task(const std::string& entry, const json::object& pipeline_override)
{
    return MaaTaskId();
}

MaaRecoId RemoteContext::run_recognition(const std::string& entry, const json::object& pipeline_override, const cv::Mat& image)
{
    return MaaRecoId();
}

MaaNodeId RemoteContext::run_action(
    const std::string& entry,
    const json::object& pipeline_override,
    const cv::Rect& box,
    const std::string& reco_detail)
{
    return MaaNodeId();
}

bool RemoteContext::override_pipeline(const json::object& pipeline_override)
{
    return false;
}

bool RemoteContext::override_next(const std::string& node_name, const std::vector<std::string>& next)
{
    return false;
}

MaaContext* RemoteContext::clone() const
{
    return nullptr;
}

MaaTaskId RemoteContext::task_id() const
{
    return MaaTaskId();
}

MaaTasker* RemoteContext::tasker() const
{
    return nullptr;
}

MAA_AGENT_SERVER_NS_END
