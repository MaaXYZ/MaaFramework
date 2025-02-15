#pragma once

#include <optional>

#include <meojson/json.hpp>

#include "MaaFramework/MaaDef.h"
#include "Utils/NoWarningCVMat.hpp"

MAA_TASK_NS_BEGIN

struct RecoResult
{
    MaaRecoId reco_id = 0;
    std::string name;
    std::string algorithm;
    std::optional<cv::Rect> box = std::nullopt;
    json::value detail;
    cv::Mat raw;
    std::vector<cv::Mat> draws;
};

struct NodeDetail
{
    MaaNodeId node_id = 0;
    std::string name;
    MaaRecoId reco_id = 0;
    bool completed = false;
};

struct TaskDetail
{
    MaaTaskId task_id = 0;
    std::string entry;
    std::vector<MaaNodeId> node_ids;
    MaaStatus status = MaaStatus_Invalid;
};

MAA_TASK_NS_END
