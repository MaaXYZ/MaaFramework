#pragma once

#include <optional>

#include <meojson/json.hpp>

#include "MaaFramework/MaaDef.h"
#include "Resource/PipelineTypes.h"
#include "Utils/NoWarningCVMat.hpp"

MAA_TASK_NS_BEGIN

struct RecoResult
{
    MaaRecoId uid = 0;
    std::string name;
    std::optional<cv::Rect> hit = std::nullopt;
    json::value detail;
    cv::Mat raw;
    std::vector<cv::Mat> draws;
};

struct NodeDetail
{
    std::string name;
    MaaRecoId reco_uid = 0;
    bool action_completed = false;
};

struct TaskDetail
{
    std::string entry;
    std::vector<MaaNodeId> node_ids;
};

MAA_TASK_NS_END
