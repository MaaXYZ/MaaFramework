#pragma once

#include <optional>

#include <meojson/json.hpp>

#include "MaaFramework/MaaDef.h"
#include "MaaUtils/NoWarningCVMat.hpp"

#include "Common/Conf.h"

MAA_TASK_NS_BEGIN

struct RecoResult
{
    MaaRecoId reco_id = MaaInvalidId;
    std::string name;
    std::string algorithm;
    std::optional<cv::Rect> box = std::nullopt;
    json::value detail;
    cv::Mat raw;
    std::vector<cv::Mat> draws;
};

struct ActionResult
{
    MaaActId action_id = MaaInvalidId;
    std::string name;
    std::string action;
    cv::Rect box {};
    bool success = false;
    json::value detail;
};

struct NodeDetail
{
    MaaNodeId node_id = MaaInvalidId;
    std::string name;
    MaaRecoId reco_id = MaaInvalidId;
    MaaActId action_id = MaaInvalidId;
    bool completed = false;
};

struct TaskDetail
{
    MaaTaskId task_id = MaaInvalidId;
    std::string entry;
    std::vector<MaaNodeId> node_ids;
    MaaStatus status = MaaStatus_Invalid;
};

MAA_TASK_NS_END
