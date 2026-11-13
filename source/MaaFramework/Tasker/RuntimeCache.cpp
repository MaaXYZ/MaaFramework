#include "RuntimeCache.h"

#include <algorithm>
#include <limits>
#include <mutex>

#include "Global/OptionMgr.h"
#include "MaaUtils/Logger.h"

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
        LogWarn << "uid is invalid";
        return std::nullopt;
    }

    std::shared_lock lock(reco_details_mutex_);

    auto it = reco_details_.find(uid);
    if (it == reco_details_.end()) {
        return std::nullopt;
    }

    MAA_TASK_NS::RecoResult result = it->second;

    auto cache_it = reco_image_cache_.find(uid);
    if (cache_it != reco_image_cache_.end()) {
        result.raw = cache_it->second.raw;
        result.draws = cache_it->second.draws;
    }

    return result;
}

std::string RuntimeCache::set_reco_detail(MaaRecoId uid, MAA_TASK_NS::RecoResult detail)
{
    if (uid == MaaInvalidId) {
        LogWarn << "uid is invalid";
        return "unavailable";
    }

    std::unique_lock lock(reco_details_mutex_);
    const MaaSize count = reco_image_count_limit_.value_or(MAA_GLOBAL_NS::OptionMgr::get_instance().reco_image_cache_limit());
    const MaaSize bytes = reco_image_byte_limit_.value_or(std::numeric_limits<MaaSize>::max());
    RecoImageCache images { std::move(detail.raw), std::move(detail.draws) };
    const MaaSize size = image_bytes(images);

    if (auto existing = reco_image_cache_.find(uid); existing != reco_image_cache_.end()) {
        reco_image_bytes_ -= image_bytes(existing->second);
        reco_image_cache_.erase(existing);
        std::erase(reco_image_order_, uid);
    }
    if (size == 0) {
        detail.image_status = "not_captured";
    }
    else if (count == 0 || bytes == 0) {
        detail.image_status = "disabled";
    }
    else if (size > bytes) {
        detail.image_status = "budget_exceeded";
    }
    else {
        evict_reco_image_cache_if_needed(count - 1, bytes - size);
        reco_image_cache_.insert_or_assign(uid, std::move(images));
        reco_image_order_.push_back(uid);
        reco_image_bytes_ += size;
        detail.image_status = "available";
    }

    // A moved-from container may retain capacity on some standard libraries.
    MAA_TASK_NS::ImageEncodedBuffer().swap(detail.raw);
    decltype(detail.draws)().swap(detail.draws);
    const auto status = detail.image_status;
    reco_details_.insert_or_assign(uid, std::move(detail));
    return status;
}

MaaSize RuntimeCache::image_bytes(const RecoImageCache& images)
{
    MaaSize size = images.raw.size();
    for (const auto& draw : images.draws) {
        size += draw.size();
    }
    return size;
}

void RuntimeCache::set_image_limit(MaaTaskerOption key, MaaSize limit)
{
    std::unique_lock lock(reco_details_mutex_);
    if (key == MaaTaskerOption_RecoImageCacheLimit) {
        reco_image_count_limit_ = limit;
    }
    else if (key == MaaTaskerOption_RecoImageCacheBytes) {
        reco_image_byte_limit_ = limit;
    }
    evict_reco_image_cache_if_needed(
        reco_image_count_limit_.value_or(MAA_GLOBAL_NS::OptionMgr::get_instance().reco_image_cache_limit()),
        reco_image_byte_limit_.value_or(std::numeric_limits<MaaSize>::max()));
}

void RuntimeCache::evict_reco_image_cache_if_needed(MaaSize count, MaaSize bytes)
{
    while ((reco_image_cache_.size() > count || reco_image_bytes_ > bytes) && !reco_image_order_.empty()) {
        const MaaRecoId oldest = reco_image_order_.front();
        reco_image_order_.pop_front();
        auto it = reco_image_cache_.find(oldest);
        if (it == reco_image_cache_.end()) {
            continue;
        }
        reco_image_bytes_ -= image_bytes(it->second);
        reco_image_cache_.erase(it);
        if (auto detail = reco_details_.find(oldest); detail != reco_details_.end()) {
            detail->second.image_status = "evicted";
        }
    }
}

std::optional<MAA_TASK_NS::ActionResult> RuntimeCache::get_action_result(MaaActId uid) const
{
    if (uid == MaaInvalidId) {
        LogWarn << "uid is invalid";
        return std::nullopt;
    }

    std::shared_lock lock(action_details_mutex_);

    auto it = action_details_.find(uid);
    if (it == action_details_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void RuntimeCache::set_action_detail(MaaActId uid, MAA_TASK_NS::ActionResult detail)
{
    if (uid == MaaInvalidId) {
        LogError << "uid is invalid";
        return;
    }

    std::unique_lock lock(action_details_mutex_);

    action_details_.insert_or_assign(uid, std::move(detail));
}

std::optional<MAA_TASK_NS::WaitFreezesDetail> RuntimeCache::get_wf_detail(MaaWfId uid) const
{
    if (uid == MaaInvalidId) {
        LogWarn << "uid is invalid";
        return std::nullopt;
    }

    std::shared_lock lock(wf_details_mutex_);

    auto it = wf_details_.find(uid);
    if (it == wf_details_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void RuntimeCache::set_wf_detail(MaaWfId uid, MAA_TASK_NS::WaitFreezesDetail detail)
{
    if (uid == MaaInvalidId) {
        LogError << "uid is invalid";
        return;
    }

    std::unique_lock lock(wf_details_mutex_);

    wf_details_.insert_or_assign(uid, std::move(detail));
}

std::optional<MAA_TASK_NS::NodeDetail> RuntimeCache::get_node_detail(MaaNodeId uid) const
{
    if (uid == MaaInvalidId) {
        LogWarn << "uid is invalid";
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
        LogError << "uid is invalid";
        return;
    }

    std::unique_lock lock(node_details_mutex_);

    node_details_.insert_or_assign(uid, std::move(detail));
}

std::optional<MAA_TASK_NS::TaskDetail> RuntimeCache::get_task_detail(MaaTaskId uid) const
{
    if (uid == MaaInvalidId) {
        LogWarn << "uid is invalid";
        return std::nullopt;
    }

    std::shared_lock lock(task_details_mutex_);

    auto it = task_details_.find(uid);
    if (it == task_details_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void RuntimeCache::set_task_status(MaaTaskId uid, const std::string& entry, MaaStatus status)
{
    if (uid == MaaInvalidId) {
        LogError << "uid is invalid";
        return;
    }

    std::unique_lock lock(task_details_mutex_);

    auto [it, inserted] = task_details_.try_emplace(uid, MAA_TASK_NS::TaskDetail { .task_id = uid, .entry = entry });
    it->second.status = status;
    if (status == MaaStatus_Running) {
        running_task_ids_.insert(uid);
    }
    else {
        running_task_ids_.erase(uid);
    }
}

void RuntimeCache::append_task_node(MaaTaskId uid, const std::string& entry, MaaNodeId node_id)
{
    if (uid == MaaInvalidId || node_id == MaaInvalidId) {
        LogError << "uid or node_id is invalid" << VAR(uid) << VAR(node_id);
        return;
    }

    std::unique_lock lock(task_details_mutex_);

    auto [it, inserted] =
        task_details_.try_emplace(uid, MAA_TASK_NS::TaskDetail { .task_id = uid, .entry = entry, .status = MaaStatus_Running });
    it->second.node_ids.emplace_back(node_id);
}

void RuntimeCache::clear(const std::set<MaaTaskId>& retained_task_ids)
{
    LogInfo;

    {
        std::unique_lock lock(latest_nodes_mutex_);
        latest_nodes_.clear();
    }
    {
        std::unique_lock lock(reco_details_mutex_);
        reco_details_.clear();
        reco_image_cache_.clear();
        reco_image_order_.clear();
        reco_image_bytes_ = 0;
    }
    {
        std::unique_lock lock(action_details_mutex_);
        action_details_.clear();
    }
    {
        std::unique_lock lock(wf_details_mutex_);
        wf_details_.clear();
    }
    {
        std::unique_lock lock(node_details_mutex_);
        node_details_.clear();
    }
    {
        std::unique_lock lock(task_details_mutex_);
        // Context recognition/action histories have no runner lifecycle of their own.
        std::erase_if(task_details_, [this, &retained_task_ids](const auto& item) {
            return !retained_task_ids.contains(item.first) && !running_task_ids_.contains(item.first);
        });
        for (auto& [id, detail] : task_details_) {
            // Release the history storage, while keeping callbacks' task metadata available.
            std::vector<MaaNodeId>().swap(detail.node_ids);
        }
    }
}

MAA_NS_END
