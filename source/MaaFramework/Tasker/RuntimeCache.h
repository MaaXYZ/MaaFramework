#pragma once

#include <deque>
#include <map>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "Common/TaskResultTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"

#include "Common/Conf.h"

MAA_NS_BEGIN

class RuntimeCache
{
public:
    std::optional<MaaNodeId> get_latest_node(const std::string& name) const;
    void set_latest_node(const std::string& name, MaaNodeId id);

    std::optional<MAA_TASK_NS::RecoResult> get_reco_result(MaaRecoId uid) const;
    void set_reco_detail(MaaRecoId uid, MAA_TASK_NS::RecoResult);

    std::optional<MAA_TASK_NS::ActionResult> get_action_result(MaaActId uid) const;
    void set_action_detail(MaaActId uid, MAA_TASK_NS::ActionResult);

    std::optional<MAA_TASK_NS::NodeDetail> get_node_detail(MaaNodeId uid) const;
    void set_node_detail(MaaNodeId uid, MAA_TASK_NS::NodeDetail detail);

    std::optional<MAA_TASK_NS::TaskDetail> get_task_detail(MaaTaskId uid) const;
    void set_task_detail(MaaTaskId uid, MAA_TASK_NS::TaskDetail detail);

    void clear();

private:
    struct RecoImageCache
    {
        MAA_TASK_NS::ImageEncodedBuffer raw;
        std::vector<MAA_TASK_NS::ImageEncodedBuffer> draws;
    };

    void evict_reco_image_cache_if_needed(size_t limit);

    std::map<std::string, MaaNodeId> latest_nodes_;
    mutable std::shared_mutex latest_nodes_mutex_;

    std::map<MaaRecoId, MAA_TASK_NS::RecoResult> reco_details_;
    std::unordered_map<MaaRecoId, RecoImageCache> reco_image_cache_;
    std::deque<MaaRecoId> reco_image_order_;
    mutable std::shared_mutex reco_details_mutex_;

    std::map<MaaActId, MAA_TASK_NS::ActionResult> action_details_;
    mutable std::shared_mutex action_details_mutex_;

    std::map<MaaNodeId, MAA_TASK_NS::NodeDetail> node_details_;
    mutable std::shared_mutex node_details_mutex_;

    std::map<MaaTaskId, MAA_TASK_NS::TaskDetail> task_details_;
    mutable std::shared_mutex task_details_mutex_;
};

MAA_NS_END
