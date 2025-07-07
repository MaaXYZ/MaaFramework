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

struct JDirectHit
{
    json::value to_json() const { return json::object(); }
};

struct JTemplateMatch
{
    JTarget roi;
    JRect roi_offset;
    std::vector<std::string> template_;
    std::vector<double> threshold;
    std::string order_by;
    int index = 0;
    int method = 0;
    bool green_mask = false;

    MEO_TOJSON(roi, roi_offset, MEO_KEY("template") template_, threshold, order_by, index, method, green_mask);
};

struct JFeatureMatch
{
    JTarget roi;
    JRect roi_offset;
    std::vector<std::string> template_;
    int count = 0;
    std::string order_by;
    int index = 0;
    bool green_mask = false;
    std::string detector;
    double ratio = 0;

    MEO_TOJSON(roi, roi_offset, MEO_KEY("template") template_, count, order_by, index, green_mask, detector, ratio);
};

struct JColorMatch
{
    JTarget roi;
    JRect roi_offset;
    int method = 0;
    std::vector<std::vector<int>> lower;
    std::vector<std::vector<int>> upper;
    int count = 0;
    std::string order_by;
    int index = 0;
    bool connected = false;

    MEO_TOJSON(roi, roi_offset, method, lower, upper, count, order_by, index, connected);
};

struct JOCR
{
    JTarget roi;
    JRect roi_offset;
    std::vector<std::string> expected;
    double threshold = 0;
    std::vector<std::pair<std::string, std::string>> replace;
    std::string order_by;
    int index = 0;
    bool only_rec = false;
    std::string model;

    MEO_TOJSON(roi, roi_offset, expected, threshold, replace, order_by, index, only_rec, model);
};

struct JNeuralNetworkClassify
{
    JTarget roi;
    JRect roi_offset;
    std::vector<std::string> labels;
    std::string model;
    std::vector<int> expected;
    std::string order_by;
    int index = 0;

    MEO_TOJSON(roi, roi_offset, labels, model, expected, order_by, index);
};

struct JNeuralNetworkDetect
{
    JTarget roi;
    JRect roi_offset;
    std::vector<std::string> labels;
    std::string model;
    std::vector<int> expected;
    std::vector<double> threshold;
    std::string order_by;
    int index = 0;

    MEO_TOJSON(roi, roi_offset, labels, model, expected, threshold, order_by, index);
};

struct JCustomRecognition
{
    JTarget roi;
    JRect roi_offset;
    std::string custom_recognition;
    json::value custom_recognition_param;

    MEO_TOJSON(roi, roi_offset, custom_recognition, custom_recognition_param);
};

using JRecognitionParam = std::
    variant<JDirectHit, JTemplateMatch, JFeatureMatch, JColorMatch, JOCR, JNeuralNetworkClassify, JNeuralNetworkDetect, JCustomRecognition>;

struct JRecognition
{
    std::string type;
    JRecognitionParam param;

    MEO_TOJSON(type, param);
};

struct JDoNothing
{
    json::value to_json() const { return json::object(); }
};

struct JClick
{
    JTarget target;
    JRect target_offset {};

    MEO_TOJSON(target, target_offset);
};

struct JLongPress
{
    JTarget target;
    JRect target_offset {};
    int64_t duration = 0;
    MEO_TOJSON(target, target_offset, duration);
};

struct JSwipe
{
    int64_t starting = 0;
    JTarget begin;
    JRect begin_offset {};
    JTarget end;
    JRect end_offset {};
    int64_t duration = 0;
    MEO_TOJSON(starting, begin, begin_offset, end, end_offset, duration);
};

struct JMultiSwipe
{
    std::vector<JSwipe> swipes;

    MEO_TOJSON(swipes);
};

struct JKey
{
    std::vector<int> key;

    MEO_TOJSON(key);
};

struct JInputText
{
    std::string input_text;

    MEO_TOJSON(input_text);
};

struct JStartApp
{
    std::string package;

    MEO_TOJSON(package);
};

struct JStopApp
{
    std::string package;

    MEO_TOJSON(package);
};

struct JStopTask
{
    json::value to_json() const { return json::object(); }
};

struct JCommand
{
    std::string exec;
    std::vector<std::string> args;
    bool detach = false;

    MEO_TOJSON(exec, args, detach);
};

struct JCustomAction
{
    JTarget target;
    JRect target_offset {};
    std::string custom_action;
    json::value custom_action_param;

    MEO_TOJSON(custom_action, custom_action_param);
};

using JActionParam =
    std::variant<JDoNothing, JClick, JLongPress, JSwipe, JMultiSwipe, JKey, JInputText, JStartApp, JStopApp, JCommand, JCustomAction>;

struct JAction
{
    std::string type;
    JActionParam param;

    MEO_TOJSON(type, param);
};

struct JWaitFreezes
{
    int64_t time = 0;
    JTarget target;
    JRect target_offset {};
    double threshold = 0;
    int method = 0;
    int64_t rate_limit = 0;
    int64_t timeout = 0;

    MEO_TOJSON(time, target, target_offset, threshold, method, rate_limit, timeout);
};

struct JPipelineData
{
    JRecognition recognition;
    JAction action;
    std::vector<std::string> next;
    std::vector<std::string> interrupt;
    bool is_sub = false;
    int64_t rate_limit = 0;
    int64_t timeout = 0;
    std::vector<std::string> on_error;
    bool inverse = false;
    bool enabled = false;
    int64_t pre_delay = 0;
    int64_t post_delay = 0;
    JWaitFreezes pre_wait_freezes;
    JWaitFreezes post_wait_freezes;
    json::value focus;

    MEO_TOJSON(
        recognition,
        action,
        next,
        interrupt,
        is_sub,
        rate_limit,
        timeout,
        on_error,
        inverse,
        enabled,
        pre_delay,
        post_delay,
        pre_wait_freezes,
        post_wait_freezes,
        focus);
};
} // namespace PipelineV2

MAA_RES_NS_END
