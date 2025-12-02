#pragma once

#include <map>
#include <optional>
#include <shared_mutex>

#include "Common/TaskResultTypes.h"

#include "Common/Conf.h"

MAA_CTRL_NS_BEGIN

class ActionCache
{
public:
    std::optional<MAA_TASK_NS::ActionResult> get_action_result(MaaActId uid) const;
    void set_action_detail(MaaActId uid, MAA_TASK_NS::ActionResult detail);

    void clear();

private:
    std::map<MaaActId, MAA_TASK_NS::ActionResult> action_details_;
    mutable std::shared_mutex action_details_mutex_;
};

MAA_CTRL_NS_END
