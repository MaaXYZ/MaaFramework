#pragma once

#include "Common/MaaConf.h"
#include "Utils/NoWarningCVMat.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#define MAA_PIPELINE_TASK_NS MAA_TASK_NS::Pipeline
#define MAA_PIPELINE_TASK_NS_BEGIN \
    namespace MAA_PIPELINE_TASK_NS \
    {
#define MAA_PIPELINE_TASK_NS_END }

MAA_PIPELINE_TASK_NS_BEGIN

namespace Vision
{
enum class Type
{
    Invalid = 0,
    DirectHit,
    TemplateMatch,
    OcrPipeline,
    OcrRec,
    FreezesWait,
};

struct TemplateMatchParams
{
    std::vector<std::string> templ_names;
    std::vector<double> thresholds;
    bool green_mask = false;
};

struct OcrParams
{
    std::vector<std::string> text;
    bool full_match = false;

    std::unordered_map<std::string, std::string> replace;
    bool replace_full = false;
};

struct FreezesWaitParams
{
    double threshold = 0.0;
    int wait_time = 0;
};
} // namespace Vision

namespace Action
{
enum class Type
{
    Invalid = 0,
    DoNothing,
    ClickSelf,
    ClickRegion,
    SwipeSelf,
    SwipeRegion,
};

struct ClickParams
{
    cv::Point point_move {};
    cv::Rect rect_move {};
};

struct ClickRegionParams
{
    cv::Rect region {};
};

struct SwipeRegionParams
{
    std::vector<cv::Point> points;
    std::vector<int> delays;
};

struct SwipeSelfParams
{
    // TODO
};
} // namespace Action

enum class NextMode
{
    Invalid = 0,
    FindFirst,
    Foreach,
};

struct Data
{
    std::string name;
    std::vector<std::string> base;
    bool main_mission = false;

    Vision::Type vision_type = Vision::Type::Invalid;
    std::variant<Vision::TemplateMatchParams, Vision::OcrParams, Vision::FreezesWaitParams> vision_params;

    cv::Rect roi {};
    bool cache = false;
    int retry_times = INT_MAX;

    std::vector<std::string> next;
    NextMode next_mode = NextMode::Invalid;

    std::vector<std::string> overflow_next;
    NextMode overflow_next_mode = NextMode::Invalid;

    std::vector<std::string> error_next;
    NextMode error_next_mode = NextMode::Invalid;

    Action::Type action_type = Action::Type::Invalid;
    std::variant<Action::ClickParams, Action::ClickRegionParams, Action::SwipeRegionParams> action_params;

    int pre_delay = 0;
    int post_delay = 0;
};

MAA_PIPELINE_TASK_NS_END
