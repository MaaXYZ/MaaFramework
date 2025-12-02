#pragma once

#include <map>
#include <optional>
#include <shared_mutex>

#include "Common/TaskResultTypes.h"

#include "Common/Conf.h"

MAA_RES_NS_BEGIN

class RecoCache
{
public:
    std::optional<MAA_TASK_NS::RecoResult> get_reco_result(MaaRecoId uid) const;
    void set_reco_detail(MaaRecoId uid, MAA_TASK_NS::RecoResult detail);

    void clear();

private:
    std::map<MaaRecoId, MAA_TASK_NS::RecoResult> reco_details_;
    mutable std::shared_mutex reco_details_mutex_;
};

MAA_RES_NS_END
