#pragma once

#include "Common/MaaConf.h"
#include "Utils/NoWarningCVMat.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <chrono>

#include "Vision/VisionTypes.h"

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
    OcrDetAndRec,
    OcrOnlyRec,
    FreezesWait,
};

using Params = std::variant<MAA_VISION_NS::DirectHitParams, MAA_VISION_NS::TemplMatchingParams,
                            MAA_VISION_NS::OcrParams, MAA_VISION_NS::FreezesWaitingParams>;
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

struct TaskData
{
    std::string name;
    std::vector<std::string> base;
    bool checkpoint = false;

    Recognition::Type rec_type = Recognition::Type::Invalid;
    Recognition::Params rec_params;

    bool cache = false;

    Action::Type action_type = Action::Type::Invalid;
    Action::Params action_params;
    std::vector<std::string> next;

    std::chrono::milliseconds timeout;
    std::vector<std::string> timeout_next;

    uint times_limit = UINT_MAX;
    std::vector<std::string> runout_next;

    std::chrono::milliseconds pre_delay;
    std::chrono::milliseconds post_delay;

    bool notify = false;
};

MAA_PIPELINE_RES_NS_END
