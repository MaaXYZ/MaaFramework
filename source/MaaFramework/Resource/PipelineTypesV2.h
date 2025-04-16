#pragma once

#include <array>
#include <string>
#include <vector>

#include <meojson/json.hpp>

#include "Conf/Conf.h"

MAA_RES_NS_BEGIN

namespace PipelineV2
{

using JRect = std::array<int, 4>;
using JTarget = std::variant<bool, std::string, JRect>;

struct JRecognition;
struct JAction;
struct JWaitFreezes;

struct JPipelineData
{
    JRecognition recognition;
    JAction action;
    std::variant<std::vector<std::string>, std::string> next;
    std::variant<std::vector<std::string>, std::string> interrupt;
    bool is_sub = false;
    uint32_t rate_limit = 0;
    uint32_t timeout = 0;
    std::variant<std::string, std::vector<std::string>> on_error;
    bool inverse = false;
    bool enabled = false;
    uint32_t pre_delay = 0;
    uint32_t post_delay = 0;
    std::variant<JWaitFreezes, uint32_t> pre_wait_freezes;
    std::variant<JWaitFreezes, uint32_t> post_wait_freezes;
    json::value focus;

    MEO_JSONIZATION(
        MEO_OPT recognition,
        MEO_OPT action,
        MEO_OPT next,
        MEO_OPT interrupt,
        MEO_OPT is_sub,
        MEO_OPT rate_limit,
        MEO_OPT timeout,
        MEO_OPT on_error,
        MEO_OPT inverse,
        MEO_OPT enabled,
        MEO_OPT pre_delay,
        MEO_OPT post_delay,
        MEO_OPT pre_wait_freezes,
        MEO_OPT post_wait_freezes,
        MEO_OPT focus);
};

struct JTemplateMatch;
struct JFeatureMatch;
struct JColorMatch;
struct JOCR;
struct JNeuralNetworkClassify;
struct JNeuralNetworkDetect;
struct JCustomRecognition;

struct JRecognition
{
    std::string name;
    std::variant<JTemplateMatch, JFeatureMatch, JColorMatch, JOCR, JNeuralNetworkClassify, JNeuralNetworkDetect, JCustomRecognition> data;

    MEO_TOJSON(name, data);
    MEO_CHECKJSON(name, MEO_OPT data);

    bool from_json(const json::value& in)
    {
        // TODO
    }
};

struct JDoNothing;
struct JClick;
struct JSwipe;
struct JMultiSwipe;
struct JKey;
struct JInputText;
struct JStartApp;
struct JStopApp;
struct JCommand;
struct JCustomAction;

struct JAction
{
    std::string name;
    std::variant<JDoNothing, JClick, JSwipe, JMultiSwipe, JKey, JMultiSwipe, JKey, JInputText, JStartApp, JStopApp, JCommand, JCustomAction>
        data;

    MEO_TOJSON(name, data);
    MEO_CHECKJSON(name, MEO_OPT data);

    bool from_json(const json::value& in)
    {
        // TODO
    }
};

struct JTemplateMatch
{
    JTarget roi;
    JRect roi_offset;
    std::variant<std::vector<std::string>, std::string> template_path;
    std::variant<std::vector<double>, double> threshold;
    std::string order_by;
    int index = 0;
    int method = 0;
    bool green_mask = false;

    MEO_JSONIZATION(
        MEO_OPT roi,
        MEO_OPT roi_offset,
        MEO_OPT template_path,
        MEO_OPT threshold,
        MEO_OPT order_by,
        MEO_OPT index,
        MEO_OPT method,
        MEO_OPT green_mask);
};

struct JWaitFreezes
{
    uint32_t time = 0;
    JTarget target;
    JRect target_offset {};
    double threshold = 0;
    int method = 0;
    uint32_t rate_limit = 0;
    uint32_t timeout = 0;

    MEO_JSONIZATION(
        MEO_OPT time,
        MEO_OPT target,
        MEO_OPT target_offset,
        MEO_OPT threshold,
        MEO_OPT method,
        MEO_OPT rate_limit,
        MEO_OPT timeout);
};

}

MAA_RES_NS_END
