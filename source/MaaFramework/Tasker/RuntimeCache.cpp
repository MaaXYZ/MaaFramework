#include "RuntimeCache.h"

MAA_NS_BEGIN

cv::Rect RuntimeCache::get_pre_box(const std::string& task_name) const
{
    auto it = pre_boxes_.find(task_name);
    if (it == pre_boxes_.end()) {
        return {};
    }
    return it->second;
}

void RuntimeCache::set_pre_box(std::string task_name, cv::Rect box)
{
    pre_boxes_.insert_or_assign(std::move(task_name), std::move(box));
}

std::optional<MAA_TASK_NS::RecoResult> RuntimeCache::get_reco_result(MaaRecoId uid) const
{
    auto it = reco_details_.find(uid);
    if (it == reco_details_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void RuntimeCache::add_reco_detail(MaaRecoId uid, MAA_TASK_NS::RecoResult detail)
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

void RuntimeCache::add_node_detail(MaaNodeId uid, MAA_TASK_NS::NodeDetail detail)
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

void RuntimeCache::add_task_detail(MaaTaskId uid, MAA_TASK_NS::TaskDetail detail)
{
    task_details_.insert_or_assign(uid, std::move(detail));
}

void RuntimeCache::clear()
{
    pre_boxes_.clear();
    reco_details_.clear();
    node_details_.clear();
    task_details_.clear();
}

MAA_NS_END
