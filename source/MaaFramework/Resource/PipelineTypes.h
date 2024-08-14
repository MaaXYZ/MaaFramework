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
    MAA_VISION_NS::CustomRecognizerParam>;
} // namespace Recognition

namespace Action
{
enum class Type
{
    Invalid = 0,
    DoNothing,
    Click,
    Swipe,
    Key,
    Text,
    StartApp,
    StopApp,
    Custom,
    StopTask,
};

struct Target
{
    enum class Type
    {
        Invalid = 0,
        Self,
        PreTask,
        Region,
    };

    Type type = Type::Self;
    std::variant<std::monostate, std::string, cv::Rect> param;
    cv::Rect offset {};
};

using TargetParam = std::variant<std::monostate, std::string, cv::Rect>;

struct ClickParam
{
    Target target;
};

struct SwipeParam
{
    Target begin;
    Target end;

    uint duration = 200;
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

struct CustomParam
{
    json::value custom_param;
};

using Param = std::variant<std::monostate, ClickParam, SwipeParam, KeyParam, TextParam, AppParam, CustomParam>;
} // namespace Action

struct WaitFreezesParam
{
    std::chrono::milliseconds time = std::chrono::milliseconds(0);

    Action::Target target;

    double threshold = 0.95;
    int method = MAA_VISION_NS::TemplateMatcherParam::kDefaultMethod;
};

struct PipelineData
{
    struct NextObject
    {
        enum class ThenGotoLabel
        {
            None,
            Head,
            Current,
            Following,
        };

        std::string name;
        ThenGotoLabel then_goto = ThenGotoLabel::None;

        MEO_TOJSON(name); // for output
    };

    using NextList = std::vector<NextObject>;

    std::string name;
    bool is_sub = false; // for compatibility
    bool enabled = true;
    uint hit_limit = UINT_MAX;

    Recognition::Type rec_type = Recognition::Type::DirectHit;
    Recognition::Param rec_param = MAA_VISION_NS::DirectHitParam {};
    bool inverse = false;

    Action::Type action_type = Action::Type::DoNothing;
    Action::Param action_param;
    NextList next;

    std::chrono::milliseconds pre_delay;
    std::chrono::milliseconds post_delay;

    WaitFreezesParam pre_wait_freezes;
    WaitFreezesParam post_wait_freezes;

    bool focus = false;
};

MAA_RES_NS_END
