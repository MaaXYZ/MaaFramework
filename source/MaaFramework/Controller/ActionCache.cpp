#include "ActionCache.h"

#include <mutex>

#include "MaaUtils/Logger.h"

MAA_CTRL_NS_BEGIN

std::optional<MAA_TASK_NS::ActionResult> ActionCache::get_action_result(MaaActId uid) const
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

void ActionCache::set_action_detail(MaaActId uid, MAA_TASK_NS::ActionResult detail)
{
    if (uid == MaaInvalidId) {
        LogError << "uid is invalid";
        return;
    }

    std::unique_lock lock(action_details_mutex_);

    action_details_.insert_or_assign(uid, std::move(detail));
}

void ActionCache::clear()
{
    LogInfo;

    std::unique_lock lock(action_details_mutex_);
    action_details_.clear();
}

MAA_CTRL_NS_END
