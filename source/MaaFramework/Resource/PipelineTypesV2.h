#pragma once

#include <array>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include <meojson/json.hpp>

#include "Common/Conf.h"

MAA_RES_NS_BEGIN

namespace PipelineV2
{
using JRect = std::array<int, 4>;
using JTarget = std::variant<bool, std::string, JRect>;
using JDuration = std::variant<int64_t, std::array<int64_t, 2>>;

struct JDirectHit
{
    JTarget roi = JRect { };
    JRect roi_offset { };

    MEO_TOJSON(roi, roi_offset);
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
    std::string color_filter;

    MEO_TOJSON(roi, roi_offset, expected, threshold, replace, order_by, index, only_rec, model, color_filter);
};

struct JNeuralNetworkClassify
{
    JTarget roi;
    JRect roi_offset;
    std::vector<std::string> labels;
    std::string model;
    std::vector<std::variant<int, std::string>> expected;
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
    std::vector<std::variant<int, std::string>> expected;
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

struct JSubRecognition;

// Sub-recognition element: either a node name (string) or inline recognition (object)
using JSubRecognitionItem = std::variant<std::string, json::value>;

struct JAnd
{
    std::vector<JSubRecognitionItem> all_of;
    int box_index = 0;

    MEO_TOJSON(all_of, box_index);
};

struct JOr
{
    std::vector<JSubRecognitionItem> any_of;

    MEO_TOJSON(any_of);
};

using JRecognitionParam = std::variant<
    JDirectHit,
    JTemplateMatch,
    JFeatureMatch,
    JColorMatch,
    JOCR,
    JNeuralNetworkClassify,
    JNeuralNetworkDetect,
    JAnd,
    JOr,
    JCustomRecognition>;

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
    JRect target_offset { };
    uint32_t contact = 0;
    int32_t pressure = 1;

    MEO_TOJSON(target, target_offset, contact, pressure);
};

struct JLongPress
{
    JTarget target;
    JRect target_offset { };
    JDuration duration = int64_t { 0 };
    uint32_t contact = 0;
    int32_t pressure = 1;
    MEO_TOJSON(target, target_offset, duration, contact, pressure);
};

struct JSwipe
{
    JDuration starting = int64_t { 0 };
    JTarget begin;
    JRect begin_offset { };
    std::vector<JTarget> end;
    std::vector<JRect> end_offset;
    std::vector<JDuration> end_hold;
    std::vector<JDuration> duration;
    bool only_hover = false;
    uint32_t contact = 0;
    int32_t pressure = 1;
    MEO_TOJSON(starting, begin, begin_offset, end, end_offset, end_hold, duration, only_hover, contact, pressure);
};

struct JMultiSwipe
{
    std::vector<JSwipe> swipes;

    MEO_TOJSON(swipes);
};

struct JTouch
{
    uint32_t contact = 0;
    JTarget target;
    JRect target_offset { };
    int32_t pressure = 0;

    MEO_TOJSON(contact, target, target_offset, pressure);
};

struct JTouchUp
{
    uint32_t contact = 0;

    MEO_TOJSON(contact);
};

struct JClickKey
{
    std::vector<int> key;

    MEO_TOJSON(key);
};

struct JLongPressKey
{
    std::vector<int> key;
    JDuration duration = int64_t { 0 };

    MEO_TOJSON(key, duration);
};

struct JKey
{
    int key = 0;

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

struct JScroll
{
    JTarget target;
    JRect target_offset { };
    int dx = 0;
    int dy = 0;

    MEO_TOJSON(target, target_offset, dx, dy);
};

struct JCommand
{
    std::string exec;
    std::vector<std::string> args;
    bool detach = false;

    MEO_TOJSON(exec, args, detach);
};

struct JShell
{
    std::string cmd;
    JDuration shell_timeout = int64_t { 20000 };

    MEO_TOJSON(cmd, shell_timeout);
};

struct JScreencap
{
    std::string filename;
    std::string format;
    int quality = 0;

    MEO_TOJSON(filename, format, quality);
};

struct JCustomAction
{
    JTarget target;
    JRect target_offset { };
    std::string custom_action;
    json::value custom_action_param;

    MEO_TOJSON(target, target_offset, custom_action, custom_action_param);
};

using JActionParam = std::variant<
    JDoNothing,
    JClick,
    JLongPress,
    JSwipe,
    JMultiSwipe,
    JTouch,
    JTouchUp,
    JClickKey,
    JLongPressKey,
    JKey,
    JInputText,
    JStartApp,
    JStopApp,
    JScroll,
    JStopTask,
    JCommand,
    JShell,
    JScreencap,
    JCustomAction>;

struct JAction
{
    std::string type;
    JActionParam param;

    MEO_TOJSON(type, param);
};

struct JWaitFreezes
{
    JDuration time = int64_t { 0 };
    JTarget target;
    JRect target_offset { };
    double threshold = 0;
    int method = 0;
    JDuration rate_limit = int64_t { 0 };
    JDuration timeout = int64_t { 0 };

    MEO_TOJSON(time, target, target_offset, threshold, method, rate_limit, timeout);
};

struct JPipelineData
{
    JRecognition recognition;
    JAction action;
    std::vector<NodeAttr> next;
    JDuration rate_limit = int64_t { 0 };
    JDuration timeout = int64_t { 0 };
    std::vector<NodeAttr> on_error;
    std::map<std::string, std::string> anchor;
    bool inverse = false;
    bool enabled = false;
    JDuration pre_delay = int64_t { 0 };
    JDuration post_delay = int64_t { 0 };
    JWaitFreezes pre_wait_freezes;
    JWaitFreezes post_wait_freezes;
    uint32_t repeat = 0;
    JDuration repeat_delay = int64_t { 0 };
    JWaitFreezes repeat_wait_freezes;
    uint32_t max_hit = 0;
    json::value focus;
    json::object attach;

    MEO_TOJSON(
        recognition,
        action,
        next,
        rate_limit,
        timeout,
        on_error,
        anchor,
        inverse,
        enabled,
        pre_delay,
        post_delay,
        pre_wait_freezes,
        post_wait_freezes,
        repeat,
        repeat_delay,
        repeat_wait_freezes,
        max_hit,
        focus,
        attach);
};
} // namespace PipelineV2

MAA_RES_NS_END
