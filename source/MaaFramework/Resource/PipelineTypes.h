#pragma once

#include <chrono>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "MaaUtils/NoWarningCVMat.hpp"
#include "Vision/VisionTypes.h"

MAA_RES_NS_BEGIN

namespace Recognition
{
enum class Type
{
    Invalid = 0,
    DirectHit,
    TemplateMatch,
    FeatureMatch,
    OCR,
    NeuralNetworkClassify,
    NeuralNetworkDetect,
    ColorMatch,
    And,
    Or,
    Custom,
};

struct AndParam;
struct OrParam;

using Param = std::variant<
    std::monostate,
    MAA_VISION_NS::DirectHitParam,
    MAA_VISION_NS::TemplateMatcherParam,
    MAA_VISION_NS::FeatureMatcherParam,
    MAA_VISION_NS::OCRerParam,
    MAA_VISION_NS::NeuralNetworkClassifierParam,
    MAA_VISION_NS::NeuralNetworkDetectorParam,
    MAA_VISION_NS::ColorMatcherParam,
    std::shared_ptr<AndParam>,
    std::shared_ptr<OrParam>,
    MAA_VISION_NS::CustomRecognitionParam>;

// Sub-recognition element for Multiple recognition
struct SubRecognition
{
    std::string sub_name;
    Type type = Type::Invalid;
    Param param;
};

// And recognition parameter (logical AND - all must match)
struct AndParam
{
    std::vector<SubRecognition> all_of;
    int box_index = 0;
};

// Or recognition parameter (logical OR - first match wins)
struct OrParam
{
    std::vector<SubRecognition> any_of;
};

inline static const std::unordered_map<std::string, Type> kTypeMap = {
    { "DirectHit", Type::DirectHit },
    { "directhit", Type::DirectHit },
    { "TemplateMatch", Type::TemplateMatch },
    { "templatematch", Type::TemplateMatch },
    { "FeatureMatch", Type::FeatureMatch },
    { "featurematch", Type::FeatureMatch },
    { "ColorMatch", Type::ColorMatch },
    { "colormatch", Type::ColorMatch },
    { "OCR", Type::OCR },
    { "ocr", Type::OCR },
    { "NeuralNetworkClassify", Type::NeuralNetworkClassify },
    { "neuralnetworkclassify", Type::NeuralNetworkClassify },
    { "nnclassify", Type::NeuralNetworkClassify },
    { "NNClassify", Type::NeuralNetworkClassify },
    { "NeuralNetworkDetect", Type::NeuralNetworkDetect },
    { "neuralnetworkdetect", Type::NeuralNetworkDetect },
    { "NNDetect", Type::NeuralNetworkDetect },
    { "nnDetect", Type::NeuralNetworkDetect },
    { "And", Type::And },
    { "and", Type::And },
    { "Or", Type::Or },
    { "or", Type::Or },
    { "Custom", Type::Custom },
    { "custom", Type::Custom },
};

inline static const std::unordered_map<Type, std::string> kTypeNameMap = {
    { Type::DirectHit, "DirectHit" },
    { Type::TemplateMatch, "TemplateMatch" },
    { Type::FeatureMatch, "FeatureMatch" },
    { Type::OCR, "OCR" },
    { Type::NeuralNetworkClassify, "NeuralNetworkClassify" },
    { Type::NeuralNetworkDetect, "NeuralNetworkDetect" },
    { Type::ColorMatch, "ColorMatch" },
    { Type::And, "And" },
    { Type::Or, "Or" },
    { Type::Custom, "Custom" },
};
} // namespace Recognition

namespace Action
{
enum class Type
{
    Invalid = 0,
    DoNothing,
    Click,
    LongPress,
    Swipe,
    MultiSwipe,
    TouchDown,
    TouchMove,
    TouchUp,
    ClickKey,
    LongPressKey,
    InputText,
    StartApp,
    StopApp,
    KeyDown,
    KeyUp,
    Scroll,
    StopTask,
    Command,
    Shell,
    Custom,
};

using TargetObj = MAA_VISION_NS::TargetObj;
using Target = MAA_VISION_NS::Target;

struct ClickParam
{
    Target target;
    uint contact = 0;
};

struct LongPressParam
{
    Target target;
    uint duration = 1000;
    uint contact = 0;
};

struct SwipeParam
{
    Target begin;
    std::vector<TargetObj> end = { {} };
    std::vector<cv::Rect> end_offset;
    std::vector<uint> end_hold;
    std::vector<uint> duration = { 200 };

    bool only_hover = false;

    uint starting = 0; // only for MultiSwipe
    uint contact = 0;
};

struct MultiSwipeParam
{
    std::vector<SwipeParam> swipes;
};

struct TouchParam
{
    uint contact = 0;
    Target target;
    int pressure = 0;
};

struct TouchUpParam
{
    uint contact = 0;
};

struct KeyParam
{
    int key = 0;
};

struct ClickKeyParam
{
    std::vector<int> keys;
};

struct LongPressKeyParam
{
    std::vector<int> keys;
    uint duration = 1000;
};

struct InputTextParam
{
    std::string text;
};

struct AppParam
{
    std::string package;
};

struct ScrollParam
{
    int dx = 0;
    int dy = 0;
};

struct ShellParam
{
    std::string cmd;
    int64_t timeout = 20000;
};

struct CommandParam
{
    std::string exec;
    std::vector<std::string> args;
    bool detach = false;
};

struct CustomParam
{
    std::string name;
    json::value custom_param;
    Target target;
};

using Param = std::variant<
    std::monostate,
    ClickParam,
    LongPressParam,
    SwipeParam,
    MultiSwipeParam,
    TouchParam,
    TouchUpParam,
    KeyParam,
    ClickKeyParam,
    LongPressKeyParam,
    InputTextParam,
    AppParam,
    ScrollParam,
    ShellParam,
    CommandParam,
    CustomParam>;

inline static const std::unordered_map<std::string, Type> kTypeMap = {
    { "DoNothing", Type::DoNothing },
    { "donothing", Type::DoNothing },
    { "Click", Type::Click },
    { "click", Type::Click },
    { "Swipe", Type::Swipe },
    { "swipe", Type::Swipe },
    { "LongPress", Type::LongPress },
    { "longpress", Type::LongPress },
    { "MultiSwipe", Type::MultiSwipe },
    { "multiswipe", Type::MultiSwipe },
    { "TouchDown", Type::TouchDown },
    { "touchdown", Type::TouchDown },
    { "TouchMove", Type::TouchMove },
    { "touchmove", Type::TouchMove },
    { "TouchUp", Type::TouchUp },
    { "touchup", Type::TouchUp },
    { "Key", Type::ClickKey },
    { "key", Type::ClickKey },
    { "ClickKey", Type::ClickKey },
    { "clickkey", Type::ClickKey },
    { "LongPressKey", Type::LongPressKey },
    { "longpresskey", Type::LongPressKey },
    { "InputText", Type::InputText },
    { "inputtext", Type::InputText },
    { "StartApp", Type::StartApp },
    { "startapp", Type::StartApp },
    { "StopApp", Type::StopApp },
    { "stopapp", Type::StopApp },
    { "KeyDown", Type::KeyDown },
    { "keydown", Type::KeyDown },
    { "KeyUp", Type::KeyUp },
    { "keyup", Type::KeyUp },
    { "Scroll", Type::Scroll },
    { "scroll", Type::Scroll },
    { "Shell", Type::Shell },
    { "shell", Type::Shell },
    { "Command", Type::Command },
    { "command", Type::Command },
    { "Custom", Type::Custom },
    { "custom", Type::Custom },
    { "StopTask", Type::StopTask },
    { "stoptask", Type::StopTask },
    { "Stop", Type::StopTask },
    { "stop", Type::StopTask },
};

inline static const std::unordered_map<Type, std::string> kTypeNameMap = {
    { Type::DoNothing, "DoNothing" },   { Type::Click, "Click" },
    { Type::LongPress, "LongPress" },   { Type::Swipe, "Swipe" },
    { Type::MultiSwipe, "MultiSwipe" }, { Type::TouchDown, "TouchDown" },
    { Type::TouchMove, "TouchMove" },   { Type::TouchUp, "TouchUp" },
    { Type::ClickKey, "ClickKey" },     { Type::LongPressKey, "LongPressKey" },
    { Type::InputText, "InputText" },   { Type::StartApp, "StartApp" },
    { Type::StopApp, "StopApp" },       { Type::KeyDown, "KeyDown" },
    { Type::KeyUp, "KeyUp" },           { Type::Scroll, "Scroll" },
    { Type::StopTask, "StopTask" },     { Type::Command, "Command" },
    { Type::Shell, "Shell" },           { Type::Custom, "Custom" },
};
} // namespace Action

struct WaitFreezesParam
{
    std::chrono::milliseconds time = std::chrono::milliseconds(0);

    Action::Target target;

    double threshold = 0.95;
    int method = MAA_VISION_NS::TemplateMatcherParam::kDefaultMethod;
    std::chrono::milliseconds rate_limit = std::chrono::milliseconds(1000);
    std::chrono::milliseconds timeout = std::chrono::milliseconds(20 * 1000);
};

struct NodeAttr
{
    std::string name;
    bool jump_back = false;
    bool anchor = false;

    MEO_JSONIZATION(name, MEO_OPT jump_back, MEO_OPT anchor);
};

struct PipelineData
{
    inline static constexpr std::string_view kNodePrefix_Ignore = "$";
    inline static constexpr std::string_view kNodeAttr_JumpBack = "[JumpBack]";
    inline static constexpr std::string_view kNodeAttr_Anchor = "[Anchor]";

    std::string name;
    bool enabled = true;

    Recognition::Type reco_type = Recognition::Type::DirectHit;
    Recognition::Param reco_param = MAA_VISION_NS::DirectHitParam {};
    bool inverse = false;

    Action::Type action_type = Action::Type::DoNothing;
    Action::Param action_param;

    std::vector<NodeAttr> next;
    std::vector<NodeAttr> on_error;
    std::vector<std::string> anchor;
    std::chrono::milliseconds rate_limit = std::chrono::milliseconds(1000);
    std::chrono::milliseconds reco_timeout = std::chrono::milliseconds(20 * 1000);

    std::chrono::milliseconds pre_delay = std::chrono::milliseconds(200);
    std::chrono::milliseconds post_delay = std::chrono::milliseconds(200);

    WaitFreezesParam pre_wait_freezes;
    WaitFreezesParam post_wait_freezes;

    uint repeat = 1;
    std::chrono::milliseconds repeat_delay = std::chrono::milliseconds(0);
    WaitFreezesParam repeat_wait_freezes;

    uint max_hit = std::numeric_limits<uint>::max();

    json::value focus;
    json::object attach;
};

MAA_RES_NS_END

MAA_NS_BEGIN

using PipelineData = MAA_RES_NS::PipelineData;
using PipelineDataMap = std::unordered_map<std::string, PipelineData>;

MAA_NS_END
