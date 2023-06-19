#pragma once

#include "Common/MaaConf.h"
#include "Utils/NoWarningCVMat.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#define MAA_PIPELINE_RES_NS MAA_RES_NS::Pipeline
#define MAA_PIPELINE_RES_NS_BEGIN \
    namespace MAA_PIPELINE_RES_NS \
    {
#define MAA_PIPELINE_RES_NS_END }

MAA_PIPELINE_RES_NS_BEGIN

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
    std::vector<std::string> templates;
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
    uint wait_time = 0;
};

using Params =
    std::variant<Recognition::TemplMatchingParams, Recognition::OcrParams, Recognition::FreezesWaitingParams>;
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
    cv::Rect rect_move {};
};

struct ClickRegionParams
{
    cv::Rect region {};
};

struct SwipeRegionParams
{
    cv::Rect begin {};
    cv::Rect end {};
    uint duration = 0;
};

struct SwipeSelfParams
{
    // TODO
};

using Params = std::variant<Action::ClickParams, Action::ClickRegionParams, Action::SwipeRegionParams>;
} // namespace Action

struct Data
{
    std::string name;
    std::vector<std::string> base;
    bool checkpoint = false;

    Recognition::Type rec_type = Recognition::Type::Invalid;
    Recognition::Params rec_params;

    std::vector<cv::Rect> roi;
    bool cache = false;

    Action::Type action_type = Action::Type::Invalid;
    Action::Params action_params;
    std::vector<std::string> next;

    uint timeout = UINT_MAX;
    std::vector<std::string> timeout_next;

    uint run_times = UINT_MAX;
    std::vector<std::string> runout_next;

    uint pre_delay = 0;
    uint post_delay = 0;

    bool notify = false;
};

MAA_PIPELINE_RES_NS_END
