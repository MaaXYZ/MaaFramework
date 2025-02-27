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

struct StartUpResponse
{
    std::string version;
    std::vector<std::string> actions;
    std::vector<std::string> recognitions;

    MessageTypePlaceholder _StartUpResponse = 1;
    MEO_JSONIZATION(version, actions, recognitions, _StartUpResponse);
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
    MEO_JSONIZATION(context_id, task_id, node_name, custom_recognition_name, custom_recognition_param, image, roi, _CustomRecognitionRequest);
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
    std::string cloned_context_id;

    MessageTypePlaceholder _ContextCloneReverseResponse = 1;
    MEO_JSONIZATION(cloned_context_id, _ContextCloneReverseResponse);
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

MAA_AGENT_NS_END
