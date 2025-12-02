#include "RecoCache.h"

#include <mutex>

#include "MaaUtils/Logger.h"

MAA_RES_NS_BEGIN

std::optional<MAA_TASK_NS::RecoResult> RecoCache::get_reco_result(MaaRecoId uid) const
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
    return it->second;
}

void RecoCache::set_reco_detail(MaaRecoId uid, MAA_TASK_NS::RecoResult detail)
{
    if (uid == MaaInvalidId) {
        LogError << "uid is invalid";
        return;
    }

    std::unique_lock lock(reco_details_mutex_);

    reco_details_.insert_or_assign(uid, std::move(detail));
}

void RecoCache::clear()
{
    LogInfo;

    std::unique_lock lock(reco_details_mutex_);
    reco_details_.clear();
}

MAA_RES_NS_END
