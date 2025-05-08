#pragma once

#include <array>
#include <string>
#include <vector>

#include <meojson/json.hpp>

#include "Conf/Conf.h"

MAA_AGENT_NS_BEGIN

// Request: client -> server
// ReverseRequest: server -> client

using MessageTypePlaceholder = int;
inline static constexpr int kProtocolVersion = 2;

struct StartUpRequest
{
    std::string version = MAA_VERSION;
    int protocol = kProtocolVersion;

    MessageTypePlaceholder _StartUpRequest = 1;
    MEO_JSONIZATION(version, protocol, _StartUpRequest);
};

struct StartUpResponse
{
    std::string version = MAA_VERSION;
    int protocol = kProtocolVersion;
    std::vector<std::string> actions;
    std::vector<std::string> recognitions;

    MessageTypePlaceholder _StartUpResponse = 1;
    MEO_JSONIZATION(version, protocol, actions, recognitions, _StartUpResponse);
};

struct ShutDownRequest
{
    MessageTypePlaceholder _ShutDownRequest = 1;
    MEO_JSONIZATION(_ShutDownRequest);
};

struct ShutDownResponse
{
    MessageTypePlaceholder _ShutDownResponse = 1;
    MEO_JSONIZATION(_ShutDownResponse);
};

struct CustomRecognitionRequest
{
    std::string context_id;
    int64_t task_id = 0;
    std::string node_name;
    std::string custom_recognition_name;
    std::string custom_recognition_param;
    std::string image;
    std::array<int32_t, 4> roi {};

    MessageTypePlaceholder _CustomRecognitionRequest = 1;
    MEO_JSONIZATION(
        context_id,
        task_id,
        node_name,
        custom_recognition_name,
        custom_recognition_param,
        image,
        roi,
        _CustomRecognitionRequest);
};

struct CustomRecognitionResponse
{
    bool ret = false;
    std::array<int32_t, 4> out_box {};
    std::string out_detail;

    MessageTypePlaceholder _CustomRecognitionResponse = 1;
    MEO_JSONIZATION(ret, out_box, out_detail, _CustomRecognitionResponse);
};

struct CustomActionRequest
{
    std::string context_id;
    int64_t task_id = 0;
    std::string node_name;
    std::string custom_action_name;
    std::string custom_action_param;
    int64_t reco_id = 0;
    std::array<int32_t, 4> box {};

    MessageTypePlaceholder _CustomActionRequest = 1;
    MEO_JSONIZATION(context_id, task_id, node_name, custom_action_name, custom_action_param, reco_id, box, _CustomActionRequest);
};

struct CustomActionResponse
{
    bool ret = false;

    MessageTypePlaceholder _CustomActionResponse = 1;
    MEO_JSONIZATION(ret, _CustomActionResponse);
};

struct ContextRunTaskReverseRequest
{
    std::string context_id;
    std::string entry;
    json::object pipeline_override;

    MessageTypePlaceholder _ContextRunTaskReverseRequest = 1;
    MEO_JSONIZATION(context_id, entry, pipeline_override, _ContextRunTaskReverseRequest);
};

struct ContextRunTaskReverseResponse
{
    int64_t task_id = 0;

    MessageTypePlaceholder _ContextRunTaskReverseResponse = 1;
    MEO_JSONIZATION(task_id, _ContextRunTaskReverseResponse);
};

struct ContextRunRecognitionReverseRequest
{
    std::string context_id;
    std::string entry;
    json::object pipeline_override;
    std::string image;

    MessageTypePlaceholder _ContextRunRecognitionReverseRequest = 1;
    MEO_JSONIZATION(context_id, entry, pipeline_override, image, _ContextRunRecognitionReverseRequest);
};

struct ContextRunRecognitionReverseResponse
{
    int64_t reco_id = 0;

    MessageTypePlaceholder _ContextRunRecognitionReverseResponse = 1;
    MEO_JSONIZATION(reco_id, _ContextRunRecognitionReverseResponse);
};

struct ContextRunActionReverseRequest
{
    std::string context_id;
    std::string entry;
    json::object pipeline_override;
    std::array<int, 4> box {};
    std::string reco_detail;

    MessageTypePlaceholder _ContextRunActionReverseRequest = 1;
    MEO_JSONIZATION(context_id, entry, pipeline_override, box, reco_detail, _ContextRunActionReverseRequest);
};

struct ContextRunActionReverseResponse
{
    int64_t node_id = 0;

    MessageTypePlaceholder _ContextRunActionReverseResponse = 1;
    MEO_JSONIZATION(node_id, _ContextRunActionReverseResponse);
};

struct ContextOverridePipelineReverseRequest
{
    std::string context_id;
    json::object pipeline_override;

    MessageTypePlaceholder _ContextOverridePipelineReverseRequest = 1;
    MEO_JSONIZATION(context_id, pipeline_override, _ContextOverridePipelineReverseRequest);
};

struct ContextOverridePipelineReverseResponse
{
    bool ret = false;

    MessageTypePlaceholder _ContextOverridePipelineReverseResponse = 1;
    MEO_JSONIZATION(ret, _ContextOverridePipelineReverseResponse);
};

struct ContextOverrideNextReverseRequest
{
    std::string context_id;
    std::string node_name;
    std::vector<std::string> next;

    MessageTypePlaceholder _ContextOverrideNextReverseRequest = 1;
    MEO_JSONIZATION(context_id, node_name, next, _ContextOverrideNextReverseRequest);
};

struct ContextOverrideNextReverseResponse
{
    bool ret = false;

    MessageTypePlaceholder _ContextOverrideNextReverseResponse = 1;
    MEO_JSONIZATION(ret, _ContextOverrideNextReverseResponse);
};

struct ContextCloneReverseRequest
{
    std::string context_id;

    MessageTypePlaceholder _ContextClone = 1;
    MEO_JSONIZATION(context_id, _ContextClone);
};

struct ContextCloneReverseResponse
{
    std::string clone_id;

    MessageTypePlaceholder _ContextCloneReverseResponse = 1;
    MEO_JSONIZATION(clone_id, _ContextCloneReverseResponse);
};

struct ContextTaskIdReverseRequest
{
    std::string context_id;

    MessageTypePlaceholder _ContextTaskIdReverseRequest = 1;
    MEO_JSONIZATION(context_id, _ContextTaskIdReverseRequest);
};

struct ContextTaskIdReverseResponse
{
    int64_t task_id = 0;

    MessageTypePlaceholder _ContextTaskIdReverseResponse = 1;
    MEO_JSONIZATION(task_id, _ContextTaskIdReverseResponse);
};

struct ContextTaskerReverseRequest
{
    std::string context_id;

    MessageTypePlaceholder _ContextTaskerReverseRequest = 1;
    MEO_JSONIZATION(context_id, _ContextTaskerReverseRequest);
};

struct ContextTaskerReverseResponse
{
    std::string tasker_id;

    MessageTypePlaceholder _ContextTaskerReverseResponse = 1;
    MEO_JSONIZATION(tasker_id, _ContextTaskerReverseResponse);
};

struct TaskerInitedReverseRequest
{
    std::string tasker_id;

    MessageTypePlaceholder _TaskerInitedReverseRequest = 1;
    MEO_JSONIZATION(tasker_id, _TaskerInitedReverseRequest);
};

struct TaskerInitedReverseResponse
{
    bool ret = false;

    MessageTypePlaceholder _TaskerInitedReverseResponse = 1;
    MEO_JSONIZATION(ret, _TaskerInitedReverseResponse);
};

struct TaskerPostTaskReverseRequest
{
    std::string tasker_id;
    std::string entry;
    json::object pipeline_override;

    MessageTypePlaceholder _TaskerPostTaskReverseRequest = 1;
    MEO_JSONIZATION(tasker_id, entry, pipeline_override, _TaskerPostTaskReverseRequest);
};

struct TaskerPostTaskReverseResponse
{
    int64_t task_id = 0;

    MessageTypePlaceholder _TaskerPostTaskReverseResponse = 1;
    MEO_JSONIZATION(task_id, _TaskerPostTaskReverseResponse);
};

struct TaskerStatusReverseRequest
{
    std::string tasker_id;
    int64_t task_id = 0;

    MessageTypePlaceholder _TaskerStatusReverseRequest = 1;
    MEO_JSONIZATION(tasker_id, task_id, _TaskerStatusReverseRequest);
};

struct TaskerStatusReverseResponse
{
    int32_t status = 0;

    MessageTypePlaceholder _TaskerStatusReverseResponse = 1;
    MEO_JSONIZATION(status, _TaskerStatusReverseResponse);
};

struct TaskerWaitReverseRequest
{
    std::string tasker_id;
    int64_t task_id = 0;

    MessageTypePlaceholder _TaskerWaitReverseRequest = 1;
    MEO_JSONIZATION(tasker_id, task_id, _TaskerWaitReverseRequest);
};

struct TaskerWaitReverseResponse
{
    int32_t status = 0;

    MessageTypePlaceholder _TaskerWaitReverseResponse = 1;
    MEO_JSONIZATION(status, _TaskerWaitReverseResponse);
};

struct TaskerRunningReverseRequest
{
    std::string tasker_id;

    MessageTypePlaceholder _TaskerRunningReverseRequest = 1;
    MEO_JSONIZATION(tasker_id, _TaskerRunningReverseRequest);
};

struct TaskerRunningReverseResponse
{
    bool ret = false;

    MessageTypePlaceholder _TaskerRunningReverseResponse = 1;
    MEO_JSONIZATION(ret, _TaskerRunningReverseResponse);
};

struct TaskerPostStopReverseRequest
{
    std::string tasker_id;

    MessageTypePlaceholder _TaskerPostStopReverseRequest = 1;
    MEO_JSONIZATION(tasker_id, _TaskerPostStopReverseRequest);
};

struct TaskerPostStopReverseResponse
{
    int64_t task_id = 0;

    MessageTypePlaceholder _TaskerPostStopReverseResponse = 1;
    MEO_JSONIZATION(task_id, _TaskerPostStopReverseResponse);
};

struct TaskerStoppingReverseRequest
{
    std::string tasker_id;

    MessageTypePlaceholder _TaskerStoppingReverseRequest = 1;
    MEO_JSONIZATION(tasker_id, _TaskerStoppingReverseRequest);
};

struct TaskerStoppingReverseResponse
{
    bool ret = false;

    MessageTypePlaceholder _TaskerStoppingReverseResponse = 1;
    MEO_JSONIZATION(ret, _TaskerStoppingReverseResponse);
};

struct TaskerResourceReverseRequest
{
    std::string tasker_id;

    MessageTypePlaceholder _TaskerResourceReverseRequest = 1;
    MEO_JSONIZATION(tasker_id, _TaskerResourceReverseRequest);
};

struct TaskerResourceReverseResponse
{
    std::string resource_id;

    MessageTypePlaceholder _TaskerResourceReverseResponse = 1;
    MEO_JSONIZATION(resource_id, _TaskerResourceReverseResponse);
};

struct TaskerControllerReverseRequest
{
    std::string tasker_id;

    MessageTypePlaceholder _TaskerControllerReverseRequest = 1;
    MEO_JSONIZATION(tasker_id, _TaskerControllerReverseRequest);
};

struct TaskerControllerReverseResponse
{
    std::string controller_id;

    MessageTypePlaceholder _TaskerControllerReverseResponse = 1;
    MEO_JSONIZATION(controller_id, _TaskerControllerReverseResponse);
};

struct TaskerClearCacheReverseRequest
{
    std::string tasker_id;

    MessageTypePlaceholder _TaskerClearCacheReverseRequest = 1;
    MEO_JSONIZATION(tasker_id, _TaskerClearCacheReverseRequest);
};

struct TaskerClearCacheReverseResponse
{
    MessageTypePlaceholder _TaskerClearCacheReverseResponse = 1;
    MEO_JSONIZATION(_TaskerClearCacheReverseResponse);
};

struct TaskerGetTaskDetailReverseRequest
{
    std::string tasker_id;
    int64_t task_id = 0;

    MessageTypePlaceholder _TaskerGetTaskDetailReverseRequest = 1;
    MEO_JSONIZATION(tasker_id, task_id, _TaskerGetTaskDetailReverseRequest);
};

struct TaskerGetTaskDetailReverseResponse
{
    bool has_value = false;
    int64_t task_id = 0;
    std::string entry;
    std::vector<int64_t> node_ids;
    int32_t status = 0;

    MessageTypePlaceholder _TaskerGetTaskDetailReverseResponse = 1;
    MEO_JSONIZATION(has_value, MEO_OPT task_id, MEO_OPT entry, MEO_OPT node_ids, MEO_OPT status, _TaskerGetTaskDetailReverseResponse);
};

struct TaskerGetNodeDetailReverseRequest
{
    std::string tasker_id;
    int64_t node_id = 0;

    MessageTypePlaceholder _TaskerGetNodeDetailReverseRequest = 1;
    MEO_JSONIZATION(tasker_id, node_id, _TaskerGetNodeDetailReverseRequest);
};

struct TaskerGetNodeDetailReverseResponse
{
    bool has_value = false;
    int64_t node_id = 0;
    std::string name;
    int64_t reco_id = 0;
    bool completed = false;

    MessageTypePlaceholder _TaskerGetNodeDetailReverseResponse = 1;
    MEO_JSONIZATION(has_value, MEO_OPT node_id, MEO_OPT name, MEO_OPT reco_id, MEO_OPT completed, _TaskerGetNodeDetailReverseResponse);
};

struct TaskerGetRecoResultReverseRequest
{
    std::string tasker_id;
    int64_t reco_id = 0;

    MessageTypePlaceholder _TaskerGetRecoResultReverseRequest = 1;
    MEO_JSONIZATION(tasker_id, reco_id, _TaskerGetRecoResultReverseRequest);
};

struct TaskerGetRecoResultReverseResponse
{
    bool has_value = false;
    int64_t reco_id = 0;
    std::string name;
    std::string algorithm;
    std::array<int32_t, 4> box {};
    json::value detail;
    std::string raw;
    std::vector<std::string> draws;

    MessageTypePlaceholder _TaskerGetRecoResultReverseResponse = 1;
    MEO_JSONIZATION(
        has_value,
        MEO_OPT reco_id,
        MEO_OPT name,
        MEO_OPT algorithm,
        MEO_OPT box,
        MEO_OPT detail,
        MEO_OPT raw,
        MEO_OPT draws,
        _TaskerGetRecoResultReverseResponse);
};

struct TaskerGetLatestNodeReverseRequest
{
    std::string tasker_id;
    std::string node_name;

    MessageTypePlaceholder _TaskerGetLatestNodeReverseRequest = 1;
    MEO_JSONIZATION(tasker_id, node_name, _TaskerGetLatestNodeReverseRequest);
};

struct TaskerGetLatestNodeReverseResponse
{
    bool has_value = false;
    int64_t latest_id = 0;

    MessageTypePlaceholder _TaskerGetLatestNodeReverseResponse = 1;
    MEO_JSONIZATION(has_value, MEO_OPT latest_id, _TaskerGetLatestNodeReverseResponse);
};

struct ResourcePostBundleReverseRequest
{
    std::string resource_id;
    std::string path;

    MessageTypePlaceholder _ResourcePostBundleReverseRequest = 1;
    MEO_JSONIZATION(resource_id, path, _ResourcePostBundleReverseRequest);
};

struct ResourcePostBundleReverseResponse
{
    int64_t res_id = 0;

    MessageTypePlaceholder _ResourcePostBundleReverseResponse = 1;
    MEO_JSONIZATION(res_id, _ResourcePostBundleReverseResponse);
};

struct ResourceStatusReverseRequest
{
    std::string resource_id;
    int64_t res_id = 0;

    MessageTypePlaceholder _ResourceStatusReverseRequest = 1;
    MEO_JSONIZATION(resource_id, res_id, _ResourceStatusReverseRequest);
};

struct ResourceStatusReverseResponse
{
    int32_t status = 0;

    MessageTypePlaceholder _ResourceStatusReverseResponse = 1;
    MEO_JSONIZATION(status, _ResourceStatusReverseResponse);
};

struct ResourceWaitReverseRequest
{
    std::string resource_id;
    int64_t res_id = 0;

    MessageTypePlaceholder _ResourceWaitReverseRequest = 1;
    MEO_JSONIZATION(resource_id, res_id, _ResourceWaitReverseRequest);
};

struct ResourceWaitReverseResponse
{
    int32_t status = 0;

    MessageTypePlaceholder _ResourceWaitReverseResponse = 1;
    MEO_JSONIZATION(status, _ResourceWaitReverseResponse);
};

struct ResourceValidReverseRequest
{
    std::string resource_id;

    MessageTypePlaceholder _ResourceValidReverseRequest = 1;
    MEO_JSONIZATION(resource_id, _ResourceValidReverseRequest);
};

struct ResourceValidReverseResponse
{
    bool ret = false;

    MessageTypePlaceholder _ResourceValidReverseResponse = 1;
    MEO_JSONIZATION(ret, _ResourceValidReverseResponse);
};

struct ResourceRunningReverseRequest
{
    std::string resource_id;

    MessageTypePlaceholder _ResourceRunningReverseRequest = 1;
    MEO_JSONIZATION(resource_id, _ResourceRunningReverseRequest);
};

struct ResourceRunningReverseResponse
{
    bool ret = false;

    MessageTypePlaceholder _ResourceRunningReverseResponse = 1;
    MEO_JSONIZATION(ret, _ResourceRunningReverseResponse);
};

struct ResourceClearReverseRequest
{
    std::string resource_id;

    MessageTypePlaceholder _ResourceClearReverseRequest = 1;
    MEO_JSONIZATION(resource_id, _ResourceClearReverseRequest);
};

struct ResourceClearReverseResponse
{
    bool ret = false;

    MessageTypePlaceholder _ResourceClearReverseResponse = 1;
    MEO_JSONIZATION(ret, _ResourceClearReverseResponse);
};

struct ResourceOverridePipelineReverseRequest
{
    std::string resource_id;
    json::object pipeline_override;

    MessageTypePlaceholder _ResourceOverridePipelineReverseRequest = 1;
    MEO_JSONIZATION(resource_id, pipeline_override, _ResourceOverridePipelineReverseRequest);
};

struct ResourceOverridePipelineReverseResponse
{
    bool ret = false;

    MessageTypePlaceholder _ResourceOverridePipelineReverseResponse = 1;
    MEO_JSONIZATION(ret, _ResourceOverridePipelineReverseResponse);
};

struct ResourceOverrideNextReverseRequest
{
    std::string resource_id;
    std::string node_name;
    std::vector<std::string> next;

    MessageTypePlaceholder _ResourceOverrideNextReverseRequest = 1;
    MEO_JSONIZATION(resource_id, node_name, next, _ResourceOverrideNextReverseRequest);
};

struct ResourceOverrideNextReverseResponse
{
    bool ret = false;

    MessageTypePlaceholder _ResourceOverrideNextReverseResponse = 1;
    MEO_JSONIZATION(ret, _ResourceOverrideNextReverseResponse);
};


struct ResourceGetHashReverseRequest
{
    std::string resource_id;

    MessageTypePlaceholder _ResourceGetHashReverseRequest = 1;
    MEO_JSONIZATION(resource_id, _ResourceGetHashReverseRequest);
};

struct ResourceGetHashReverseResponse
{
    std::string hash;

    MessageTypePlaceholder _ResourceGetHashReverseResponse = 1;
    MEO_JSONIZATION(hash, _ResourceGetHashReverseResponse);
};

struct ResourceGetNodeListReverseRequest
{
    std::string resource_id;

    MessageTypePlaceholder _ResourceGetNodeListReverseRequest = 1;
    MEO_JSONIZATION(resource_id, _ResourceGetNodeListReverseRequest);
};

struct ResourceGetNodeListReverseResponse
{
    std::vector<std::string> node_list;

    MessageTypePlaceholder _ResourceGetNodeListReverseResponse = 1;
    MEO_JSONIZATION(node_list, _ResourceGetNodeListReverseResponse);
};

struct ControllerPostConnectionReverseRequest
{
    std::string controller_id;

    MessageTypePlaceholder _ControllerPostConnectionReverseRequest = 1;
    MEO_JSONIZATION(controller_id, _ControllerPostConnectionReverseRequest);
};

struct ControllerPostConnectionReverseResponse
{
    int64_t ctrl_id = 0;

    MessageTypePlaceholder _ControllerPostConnectionReverseResponse = 1;
    MEO_JSONIZATION(ctrl_id, _ControllerPostConnectionReverseResponse);
};

struct ControllerPostClickReverseRequest
{
    std::string controller_id;
    int32_t x = 0;
    int32_t y = 0;

    MessageTypePlaceholder _ControllerPostClickReverseRequest = 1;
    MEO_JSONIZATION(controller_id, x, y, _ControllerPostClickReverseRequest);
};

struct ControllerPostClickReverseResponse
{
    int64_t ctrl_id = 0;

    MessageTypePlaceholder _ControllerPostClickReverseResponse = 1;
    MEO_JSONIZATION(ctrl_id, _ControllerPostClickReverseResponse);
};

struct ControllerPostSwipeReverseRequest
{
    std::string controller_id;
    int32_t x1 = 0;
    int32_t y1 = 0;
    int32_t x2 = 0;
    int32_t y2 = 0;
    int32_t duration = 0;

    MessageTypePlaceholder _ControllerPostSwipeReverseRequest = 1;
    MEO_JSONIZATION(controller_id, x1, y1, x2, y2, duration, _ControllerPostSwipeReverseRequest);
};

struct ControllerPostSwipeReverseResponse
{
    int64_t ctrl_id = 0;

    MessageTypePlaceholder _ControllerPostSwipeReverseResponse = 1;
    MEO_JSONIZATION(ctrl_id, _ControllerPostSwipeReverseResponse);
};

struct ControllerPostPressKeyReverseRequest
{
    std::string controller_id;
    int32_t keycode = 0;

    MessageTypePlaceholder _ControllerPostPressKeyReverseRequest = 1;
    MEO_JSONIZATION(controller_id, keycode, _ControllerPostPressKeyReverseRequest);
};

struct ControllerPostPressKeyReverseResponse
{
    int64_t ctrl_id = 0;

    MessageTypePlaceholder _ControllerPostPressKeyReverseResponse = 1;
    MEO_JSONIZATION(ctrl_id, _ControllerPostPressKeyReverseResponse);
};

struct ControllerPostInputTextReverseRequest
{
    std::string controller_id;
    std::string text;

    MessageTypePlaceholder _ControllerPostInputTextReverseRequest = 1;
    MEO_JSONIZATION(controller_id, text, _ControllerPostInputTextReverseRequest);
};

struct ControllerPostInputTextReverseResponse
{
    int64_t ctrl_id = 0;

    MessageTypePlaceholder _ControllerPostInputTextReverseResponse = 1;
    MEO_JSONIZATION(ctrl_id, _ControllerPostInputTextReverseResponse);
};

struct ControllerPostStartAppReverseRequest
{
    std::string controller_id;
    std::string intent;

    MessageTypePlaceholder _ControllerPostStartAppReverseRequest = 1;
    MEO_JSONIZATION(controller_id, intent, _ControllerPostStartAppReverseRequest);
};

struct ControllerPostStartAppReverseResponse
{
    int64_t ctrl_id = 0;

    MessageTypePlaceholder _ControllerPostStartAppReverseResponse = 1;
    MEO_JSONIZATION(ctrl_id, _ControllerPostStartAppReverseResponse);
};

struct ControllerPostStopAppReverseRequest
{
    std::string controller_id;
    std::string intent;

    MessageTypePlaceholder _ControllerPostStopAppReverseRequest = 1;
    MEO_JSONIZATION(controller_id, intent, _ControllerPostStopAppReverseRequest);
};

struct ControllerPostStopAppReverseResponse
{
    int64_t ctrl_id = 0;

    MessageTypePlaceholder _ControllerPostStopAppReverseResponse = 1;
    MEO_JSONIZATION(ctrl_id, _ControllerPostStopAppReverseResponse);
};

struct ControllerPostScreencapReverseRequest
{
    std::string controller_id;

    MessageTypePlaceholder _ControllerPostScreencapReverseRequest = 1;
    MEO_JSONIZATION(controller_id, _ControllerPostScreencapReverseRequest);
};

struct ControllerPostScreencapReverseResponse
{
    int64_t ctrl_id = 0;

    MessageTypePlaceholder _ControllerPostScreencapReverseResponse = 1;
    MEO_JSONIZATION(ctrl_id, _ControllerPostScreencapReverseResponse);
};

struct ControllerPostTouchDownReverseRequest
{
    std::string controller_id;
    int32_t contact = 0;
    int32_t x = 0;
    int32_t y = 0;
    int32_t pressure = 0;

    MessageTypePlaceholder _ControllerPostTouchDownReverseRequest = 1;
    MEO_JSONIZATION(controller_id, contact, x, y, pressure, _ControllerPostTouchDownReverseRequest);
};

struct ControllerPostTouchDownReverseResponse
{
    int64_t ctrl_id = 0;

    MessageTypePlaceholder _ControllerPostTouchDownReverseResponse = 1;
    MEO_JSONIZATION(ctrl_id, _ControllerPostTouchDownReverseResponse);
};

struct ControllerPostTouchMoveReverseRequest
{
    std::string controller_id;
    int32_t contact = 0;
    int32_t x = 0;
    int32_t y = 0;
    int32_t pressure = 0;

    MessageTypePlaceholder _ControllerPostTouchMoveReverseRequest = 1;
    MEO_JSONIZATION(controller_id, contact, x, y, pressure, _ControllerPostTouchMoveReverseRequest);
};

struct ControllerPostTouchMoveReverseResponse
{
    int64_t ctrl_id = 0;

    MessageTypePlaceholder _ControllerPostTouchMoveReverseResponse = 1;
    MEO_JSONIZATION(ctrl_id, _ControllerPostTouchMoveReverseResponse);
};

struct ControllerPostTouchUpReverseRequest
{
    std::string controller_id;
    int32_t contact = 0;

    MessageTypePlaceholder _ControllerPostTouchUpReverseRequest = 1;
    MEO_JSONIZATION(controller_id, contact, _ControllerPostTouchUpReverseRequest);
};

struct ControllerPostTouchUpReverseResponse
{
    int64_t ctrl_id = 0;

    MessageTypePlaceholder _ControllerPostTouchUpReverseResponse = 1;
    MEO_JSONIZATION(ctrl_id, _ControllerPostTouchUpReverseResponse);
};

struct ControllerStatusReverseRequest
{
    std::string controller_id;
    int64_t ctrl_id = 0;

    MessageTypePlaceholder _ControllerStatusReverseRequest = 1;
    MEO_JSONIZATION(controller_id, ctrl_id, _ControllerStatusReverseRequest);
};

struct ControllerStatusReverseResponse
{
    int32_t status = 0;

    MessageTypePlaceholder _ControllerStatusReverseResponse = 1;
    MEO_JSONIZATION(status, _ControllerStatusReverseResponse);
};

struct ControllerWaitReverseRequest
{
    std::string controller_id;
    int64_t ctrl_id = 0;

    MessageTypePlaceholder _ControllerWaitReverseRequest = 1;
    MEO_JSONIZATION(controller_id, ctrl_id, _ControllerWaitReverseRequest);
};

struct ControllerWaitReverseResponse
{
    int32_t status = 0;

    MessageTypePlaceholder _ControllerWaitReverseResponse = 1;
    MEO_JSONIZATION(status, _ControllerWaitReverseResponse);
};

struct ControllerConnectedReverseRequest
{
    std::string controller_id;

    MessageTypePlaceholder _ControllerConnectedReverseRequest = 1;
    MEO_JSONIZATION(controller_id, _ControllerConnectedReverseRequest);
};

struct ControllerConnectedReverseResponse
{
    bool ret = false;

    MessageTypePlaceholder _ControllerConnectedReverseResponse = 1;
    MEO_JSONIZATION(ret, _ControllerConnectedReverseResponse);
};

struct ControllerRunningReverseRequest
{
    std::string controller_id;

    MessageTypePlaceholder _ControllerRunningReverseRequest = 1;
    MEO_JSONIZATION(controller_id, _ControllerRunningReverseRequest);
};

struct ControllerRunningReverseResponse
{
    bool ret = false;

    MessageTypePlaceholder _ControllerRunningReverseResponse = 1;
    MEO_JSONIZATION(ret, _ControllerRunningReverseResponse);
};

struct ControllerCachedImageReverseRequest
{
    std::string controller_id;

    MessageTypePlaceholder _ControllerCachedImageReverseRequest = 1;
    MEO_JSONIZATION(controller_id, _ControllerCachedImageReverseRequest);
};

struct ControllerCachedImageReverseResponse
{
    std::string image;

    MessageTypePlaceholder _ControllerCachedImageReverseResponse = 1;
    MEO_JSONIZATION(image, _ControllerCachedImageReverseResponse);
};

struct ControllerGetUuidReverseRequest
{
    std::string controller_id;

    MessageTypePlaceholder _ControllerGetUuidReverseRequest = 1;
    MEO_JSONIZATION(controller_id, _ControllerGetUuidReverseRequest);
};

struct ControllerGetUuidReverseResponse
{
    std::string uuid;

    MessageTypePlaceholder _ControllerGetUuidReverseResponse = 1;
    MEO_JSONIZATION(uuid, _ControllerGetUuidReverseResponse);
};

struct ImageHeader
{
    std::string uuid;

    int rows = 0;
    int cols = 0;
    int type = 0;
    size_t size = 0;

    MessageTypePlaceholder _ImageHeader = 1;

    MEO_JSONIZATION(uuid, rows, cols, type, size, _ImageHeader);
};

MAA_AGENT_NS_END
