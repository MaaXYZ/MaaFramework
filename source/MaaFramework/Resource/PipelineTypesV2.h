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
    json::value to_json() const { return {}; }
};

struct JTemplateMatch
{
    JTarget roi;
    JRect roi_offset;
    std::variant<std::vector<std::string>, std::string> templates;
    std::variant<std::vector<double>, double> threshold;
    std::string order_by;
    int index = 0;
    int method = 0;
    bool green_mask = false;

    MEO_TOJSON(roi, roi_offset, templates, threshold, order_by, index, method, green_mask);
};

struct JFeatureMatch
{
    JTarget roi;
    JRect roi_offset;
    std::variant<std::vector<std::string>, std::string> templates;
    int count = 0;
    std::string order_by;
    int index = 0;
    bool green_mask = false;
    std::string detector;
    double ratio = 0;

    MEO_TOJSON(roi, roi_offset, templates, count, order_by, index, green_mask, detector, ratio);
};

struct JColorMatch
{
    JTarget roi;
    JRect roi_offset;
    int method = 0;
    std::variant<std::vector<std::vector<int>>, std::vector<int>> lower;
    std::variant<std::vector<std::vector<int>>, std::vector<int>> upper;
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
    std::variant<std::vector<std::string>, std::string> expected;
    double threshold = 0;
    std::variant<std::vector<std::pair<int, int>>, std::pair<int, int>> replace;
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
    std::variant<std::vector<int>, int> expected;
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
    std::variant<std::vector<int>, int> expected;
    std::variant<std::vector<double>, double> threshold;
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

using JRecognitionData = std::
    variant<JDirectHit, JTemplateMatch, JFeatureMatch, JColorMatch, JOCR, JNeuralNetworkClassify, JNeuralNetworkDetect, JCustomRecognition>;

struct JRecognition
{
    std::string algorithm;
    JRecognitionData data;

    MEO_TOJSON(algorithm, data);
};

struct JDoNothing
{
    json::value to_json() const { return {}; }
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
    int duration = 0;
    MEO_TOJSON(target, target_offset, duration);
};

struct JSwipe
{
    int starting = 0;
    JTarget begin;
    JRect begin_offset {};
    JTarget end;
    JRect end_offset {};
    int duration = 0;
    MEO_TOJSON(starting, begin, begin_offset, end, end_offset, duration);
};

struct JMultiSwipe
{
    std::vector<JSwipe> swipes;

    MEO_TOJSON(swipes);
};

struct JKey
{
    std::variant<std::vector<int>, int> key;

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
    json::value to_json() const { return {}; }
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

using JActionData = std::variant<JDoNothing, JClick, JSwipe, JMultiSwipe, JKey, JInputText, JStartApp, JStopApp, JCommand, JCustomAction>;

struct JAction
{
    std::string type;
    JActionData data;

    MEO_TOJSON(type, data);
};

struct JWaitFreezes
{
    int time = 0;
    JTarget target;
    JRect target_offset {};
    double threshold = 0;
    int method = 0;
    int rate_limit = 0;
    int timeout = 0;

    MEO_TOJSON(time, target, target_offset, threshold, method, rate_limit, timeout);
};

struct JPipelineData
{
    JRecognition recognition;
    JAction action;
    std::variant<std::vector<std::string>, std::string> next;
    std::variant<std::vector<std::string>, std::string> interrupt;
    bool is_sub = false;
    int rate_limit = 0;
    int timeout = 0;
    std::variant<std::vector<std::string>, std::string> on_error;
    bool inverse = false;
    bool enabled = false;
    int pre_delay = 0;
    int post_delay = 0;
    std::variant<JWaitFreezes, int> pre_wait_freezes;
    std::variant<JWaitFreezes, int> post_wait_freezes;
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
