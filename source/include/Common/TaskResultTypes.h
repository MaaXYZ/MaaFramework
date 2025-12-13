#pragma once

#include <optional>

#include "MaaFramework/MaaDef.h"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/NoWarningCVMat.hpp"

#include "Common/Conf.h"

MAA_TASK_NS_BEGIN

using ImageEncodedBuffer = std::vector<uint8_t>;

struct RecoResult
{
    MaaRecoId reco_id = MaaInvalidId;
    std::string name;
    std::string algorithm;
    std::optional<cv::Rect> box = std::nullopt;
    json::value detail;
    ImageEncodedBuffer raw;
    std::vector<ImageEncodedBuffer> draws;

    MEO_TOJSON(reco_id, name, algorithm, box, detail);
};

struct ActionResult
{
    MaaActId action_id = MaaInvalidId;
    std::string name;
    std::string action;
    cv::Rect box {};
    bool success = false;
    json::value detail;

    MEO_TOJSON(action_id, name, action, box, success, detail);
};

struct NodeDetail
{
    MaaNodeId node_id = MaaInvalidId;
    std::string name;
    MaaRecoId reco_id = MaaInvalidId;
    MaaActId action_id = MaaInvalidId;
    bool completed = false;

    MEO_TOJSON(node_id, name, reco_id, action_id, completed);
};

struct TaskDetail
{
    MaaTaskId task_id = MaaInvalidId;
    std::string entry;
    std::vector<MaaNodeId> node_ids;
    MaaStatus status = MaaStatus_Invalid;

    MEO_TOJSON(task_id, entry, node_ids, status);
};

MAA_TASK_NS_END
