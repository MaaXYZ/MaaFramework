#include "RuntimeCache.h"

MAA_NS_BEGIN

std::optional<MaaNodeId> RuntimeCache::get_latest_node(const std::string& name) const
{
    auto it = latest_nodes_.find(name);
    if (it == latest_nodes_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void RuntimeCache::set_latest_node(const std::string& name, MaaNodeId id)
{
    latest_nodes_.insert_or_assign(name, id);
}

std::optional<MAA_TASK_NS::RecoResult> RuntimeCache::get_reco_result(MaaRecoId uid) const
{
    auto it = reco_details_.find(uid);
    if (it == reco_details_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void RuntimeCache::set_reco_detail(MaaRecoId uid, MAA_TASK_NS::RecoResult detail)
{
    reco_details_.insert_or_assign(uid, std::move(detail));
}

std::optional<MAA_TASK_NS::NodeDetail> RuntimeCache::get_node_detail(MaaNodeId uid) const
{
    auto it = node_details_.find(uid);
    if (it == node_details_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void RuntimeCache::set_node_detail(MaaNodeId uid, MAA_TASK_NS::NodeDetail detail)
{
    node_details_.insert_or_assign(uid, std::move(detail));
}

std::optional<MAA_TASK_NS::TaskDetail> RuntimeCache::get_task_detail(MaaTaskId uid) const
{
    auto it = task_details_.find(uid);
    if (it == task_details_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void RuntimeCache::set_task_detail(MaaTaskId uid, MAA_TASK_NS::TaskDetail detail)
{
    task_details_.insert_or_assign(uid, std::move(detail));
}

void RuntimeCache::clear()
{
    latest_nodes_.clear();
    reco_details_.clear();
    node_details_.clear();
    task_details_.clear();
}

MAA_NS_END
