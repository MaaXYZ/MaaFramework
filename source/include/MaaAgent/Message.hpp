#pragma once

#include <array>
#include <string>
#include <vector>

#include <meojson/json.hpp>

#include "Conf/Conf.h"

MAA_AGENT_NS_BEGIN

// Request: client -> server
// ReverseRequest: server -> client

struct StartUpResponse
{
    std::string version;
    std::vector<std::string> actions;
    std::vector<std::string> recognitions;

    MEO_JSONIZATION(version, actions, recognitions);
};

struct ShutDownRequest
{
    int shut_down_request_placeholder = 0;

    MEO_JSONIZATION(shut_down_request_placeholder);
};

struct ShutDownResponse
{
    int shut_down_response_placeholder = 0;

    MEO_JSONIZATION(shut_down_response_placeholder);
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

    MEO_JSONIZATION(context_id, task_id, node_name, custom_recognition_name, custom_recognition_param, image, roi);
};

struct CustomRecognitionResponse
{
    bool ret = false;
    std::array<int32_t, 4> out_box {};
    std::string out_detail;

    MEO_JSONIZATION(ret, out_box, out_detail);
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

    MEO_JSONIZATION(context_id, task_id, node_name, custom_action_name, custom_action_param, reco_id, box);
};

struct CustomActionResponse
{
    bool ret = false;

    MEO_JSONIZATION(ret);
};

struct ContextRunTaskReverseRequest
{
    std::string context_id;
    std::string entry;
    json::object pipeline_override;

    MEO_JSONIZATION(context_id, entry, pipeline_override);
};

struct ContextRunTaskReverseResponse
{
    int64_t task_id = 0;

    MEO_JSONIZATION(task_id);
};

MAA_AGENT_NS_END
