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

namespace Recognition
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

struct TemplMatchingParams
{
    std::vector<std::string> templ_names;
    std::vector<double> thresholds;
    int method = 0;
    bool green_mask = false;
};

struct OcrParams
{
    std::vector<std::string> text;

    std::unordered_map<std::string, std::string> replace;
};

struct FreezesWaitingParams
{
    double threshold = 0.0;
    int method = 0;
    int wait_time = 0;
};
} // namespace Recognition

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
    Find,
    Foreach,
};

struct Data
{
    std::string name;
    std::vector<std::string> base;
    bool checkpoint = false;

    Recognition::Type recognition_type = Recognition::Type::Invalid;
    std::variant<Recognition::TemplMatchingParams, Recognition::OcrParams, Recognition::FreezesWaitingParams>
        recognition_params;

    cv::Rect roi {};
    bool cache = false;
    uint times = UINT_MAX;
    uint timeout = UINT_MAX;

    std::vector<std::string> next;
    NextMode next_mode = NextMode::Invalid;

    std::vector<std::string> overflow_next;
    NextMode overflow_next_mode = NextMode::Invalid;

    std::vector<std::string> timeout_next;
    NextMode timeout_next_mode = NextMode::Invalid;

    Action::Type action_type = Action::Type::Invalid;
    std::variant<Action::ClickParams, Action::ClickRegionParams, Action::SwipeRegionParams> action_params;

    int pre_delay = 0;
    int post_delay = 0;

    bool notify = false;
};

MAA_PIPELINE_TASK_NS_END
