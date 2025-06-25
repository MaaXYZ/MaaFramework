#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "Utils/NoWarningCVMat.hpp"
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
    Custom,
};

using Param = std::variant<
    std::monostate,
    MAA_VISION_NS::DirectHitParam,
    MAA_VISION_NS::TemplateMatcherParam,
    MAA_VISION_NS::FeatureMatcherParam,
    MAA_VISION_NS::OCRerParam,
    MAA_VISION_NS::NeuralNetworkClassifierParam,
    MAA_VISION_NS::NeuralNetworkDetectorParam,
    MAA_VISION_NS::ColorMatcherParam,
    MAA_VISION_NS::CustomRecognitionParam>;

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
    Key,
    Text,
    StartApp,
    StopApp,
    Command,
    Custom,
    StopTask,
};

using Target = MAA_VISION_NS::Target;

struct ClickParam
{
    Target target;
};

struct LongPressParam
{
    Target target;
    uint duration = 500;
};

struct SwipeParam
{
    Target begin;
    Target end;

    uint duration = 200;
    uint starting = 0; // only for MultiSwipe
};

struct MultiSwipeParam
{
    std::vector<SwipeParam> swipes;
};

struct KeyParam
{
    std::vector<int> keys;
};

struct TextParam
{
    std::string text;
};

struct AppParam
{
    std::string package;
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
    KeyParam,
    TextParam,
    AppParam,
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
    { "PressKey", Type::Key },
    { "presskey", Type::Key },
    { "Key", Type::Key },
    { "key", Type::Key },
    { "InputText", Type::Text },
    { "inputtext", Type::Text },
    { "Text", Type::Text },
    { "text", Type::Text },
    { "StartApp", Type::StartApp },
    { "startapp", Type::StartApp },
    { "StopApp", Type::StopApp },
    { "stopapp", Type::StopApp },
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
    { Type::DoNothing, "DoNothing" },   { Type::Click, "Click" },     { Type::LongPress, "LongPress" }, { Type::Swipe, "Swipe" },
    { Type::MultiSwipe, "MultiSwipe" }, { Type::Key, "Key" },         { Type::Text, "Text" },           { Type::StartApp, "StartApp" },
    { Type::StopApp, "StopApp" },       { Type::Command, "Command" }, { Type::Custom, "Custom" },       { Type::StopTask, "StopTask" },
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

struct PipelineData
{
    using NextList = std::vector<std::string>;

    std::string name;
    bool is_sub = false; // for compatibility with 1.x
    bool enabled = true;

    Recognition::Type reco_type = Recognition::Type::DirectHit;
    Recognition::Param reco_param = MAA_VISION_NS::DirectHitParam {};
    bool inverse = false;

    Action::Type action_type = Action::Type::DoNothing;
    Action::Param action_param;

    NextList next;
    NextList interrupt;
    NextList on_error;
    std::chrono::milliseconds rate_limit = std::chrono::milliseconds(1000);
    std::chrono::milliseconds reco_timeout = std::chrono::milliseconds(20 * 1000);

    std::chrono::milliseconds pre_delay = std::chrono::milliseconds(200);
    std::chrono::milliseconds post_delay = std::chrono::milliseconds(200);

    WaitFreezesParam pre_wait_freezes;
    WaitFreezesParam post_wait_freezes;

    json::value focus;
};

MAA_RES_NS_END

MAA_NS_BEGIN

using PipelineData = MAA_RES_NS::PipelineData;
using PipelineDataMap = std::unordered_map<std::string, PipelineData>;

MAA_NS_END
