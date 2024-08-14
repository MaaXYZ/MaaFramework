#pragma once

#include <optional>

#include <meojson/common/value.hpp>

#include "MaaFramework/MaaDef.h"
#include "Resource/PipelineTypes.h"
#include "Utils/NoWarningCVMat.hpp"

MAA_TASK_NS_BEGIN

enum class NodeStatus
{
    None,
    RunCompleted,
    OnlyRecognized,
};

struct RecoResult
{
    MaaRecoId uid = 0;
    std::string name;
    std::optional<cv::Rect> hit = std::nullopt;
    json::value detail;
    cv::Mat raw;
    std::vector<cv::Mat> draws;
};

struct HitDetail
{
    MaaRecoId reco_uid = 0;

    cv::Rect reco_hit {};
    json::value reco_detail;
    MAA_RES_NS::PipelineData pipeline_data;
};

struct NodeDetail
{
    MaaNodeId node_id = 0;

    std::string name;
    HitDetail hit;
    NodeStatus status = NodeStatus::None;
};

struct TaskDetail
{
    std::string entry;
    std::vector<MaaNodeId> node_ids;
};

MAA_TASK_NS_END
