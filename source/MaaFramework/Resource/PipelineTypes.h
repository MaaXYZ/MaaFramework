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
} // namespace Recognition

namespace Action
{
enum class Type
{
    Invalid = 0,
    DoNothing,
    Click,
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

using Param =
    std::variant<std::monostate, ClickParam, SwipeParam, MultiSwipeParam, KeyParam, TextParam, AppParam, CommandParam, CustomParam>;
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
    bool enable = true;

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
