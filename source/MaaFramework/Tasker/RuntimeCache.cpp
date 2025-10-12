#include "RuntimeCache.h"

#include <mutex>

#include "Utils/Logger.h"

MAA_NS_BEGIN

std::optional<MaaNodeId> RuntimeCache::get_latest_node(const std::string& name) const
{
    if (name.empty()) {
        LogWarn << "name is empty";
        return std::nullopt;
    }

    std::shared_lock lock(latest_nodes_mutex_);

    auto it = latest_nodes_.find(name);
    if (it == latest_nodes_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void RuntimeCache::set_latest_node(const std::string& name, MaaNodeId id)
{
    if (name.empty() || id == MaaInvalidId) {
        LogError << "name is empty or id is invalid" << VAR(name) << VAR(id);
        return;
    }

    std::unique_lock lock(latest_nodes_mutex_);

    latest_nodes_.insert_or_assign(name, id);
}

std::optional<MAA_TASK_NS::RecoResult> RuntimeCache::get_reco_result(MaaRecoId uid) const
{
    if (uid == MaaInvalidId) {
        LogWarn << "uid is invaild";
        return std::nullopt;
    }

    std::shared_lock lock(reco_details_mutex_);

    auto it = reco_details_.find(uid);
    if (it == reco_details_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void RuntimeCache::set_reco_detail(MaaRecoId uid, MAA_TASK_NS::RecoResult detail)
{
    if (uid == MaaInvalidId) {
        LogError << "uid is invaild";
        return;
    }

    std::unique_lock lock(reco_details_mutex_);

    reco_details_.insert_or_assign(uid, std::move(detail));
}

std::optional<MAA_TASK_NS::NodeDetail> RuntimeCache::get_node_detail(MaaNodeId uid) const
{
    if (uid == MaaInvalidId) {
        LogWarn << "uid is invaild";
        return std::nullopt;
    }

    std::shared_lock lock(node_details_mutex_);

    auto it = node_details_.find(uid);
    if (it == node_details_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void RuntimeCache::set_node_detail(MaaNodeId uid, MAA_TASK_NS::NodeDetail detail)
{
    if (uid == MaaInvalidId) {
        LogError << "uid is invaild";
        return;
    }

    std::unique_lock lock(node_details_mutex_);

    node_details_.insert_or_assign(uid, std::move(detail));
}

std::optional<MAA_TASK_NS::TaskDetail> RuntimeCache::get_task_detail(MaaTaskId uid) const
{
    if (uid == MaaInvalidId) {
        LogWarn << "uid is invaild";
        return std::nullopt;
    }

    std::shared_lock lock(task_details_mutex_);

    auto it = task_details_.find(uid);
    if (it == task_details_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void RuntimeCache::set_task_detail(MaaTaskId uid, MAA_TASK_NS::TaskDetail detail)
{
    if (uid == MaaInvalidId) {
        LogError << "uid is invaild";
        return;
    }

    std::unique_lock lock(task_details_mutex_);

    task_details_.insert_or_assign(uid, std::move(detail));
}

void RuntimeCache::clear()
{
    LogInfo;

    {
        std::unique_lock lock(latest_nodes_mutex_);
        latest_nodes_.clear();
    }
    {
        std::unique_lock lock(reco_details_mutex_);
        reco_details_.clear();
    }
    {
        std::unique_lock lock(node_details_mutex_);
        node_details_.clear();
    }
    {
        std::unique_lock lock(task_details_mutex_);
        task_details_.clear();
    }
}

MAA_NS_END
