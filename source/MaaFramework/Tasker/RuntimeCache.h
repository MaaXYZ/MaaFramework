#pragma once

#include <map>
#include <optional>
#include <string>

#include "Task/TaskResultTypes.h"
#include "Utils/NoWarningCVMat.hpp"

MAA_NS_BEGIN

class RuntimeCache
{
public:
    cv::Rect get_pre_box(const std::string& task_name) const;
    void set_pre_box(std::string task_name, cv::Rect box);

    std::optional<MAA_TASK_NS::RecoResult> get_reco_result(MaaRecoId uid) const;
    void add_reco_detail(MaaRecoId uid, MAA_TASK_NS::RecoResult);

    std::optional<MAA_TASK_NS::NodeDetail> get_node_detail(MaaNodeId uid) const;
    void add_node_detail(MaaNodeId uid, MAA_TASK_NS::NodeDetail detail);

    std::optional<MAA_TASK_NS::TaskDetail> get_task_detail(MaaTaskId uid) const;
    void add_task_detail(MaaTaskId uid, MAA_TASK_NS::TaskDetail detail);

    void clear();

private:
    std::map<std::string, cv::Rect> pre_boxes_;
    std::map<MaaRecoId, MAA_TASK_NS::RecoResult> reco_details_;
    std::map<MaaNodeId, MAA_TASK_NS::NodeDetail> node_details_;
    std::map<MaaTaskId, MAA_TASK_NS::TaskDetail> task_details_;
};

MAA_NS_END
