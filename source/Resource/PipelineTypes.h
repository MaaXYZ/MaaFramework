#pragma once

#include "Common/MaaConf.h"
#include "Utils/NoWarningCVMat.h"

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <meojson/json.hpp>

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
    OCR,
};

using Params = std::variant<std::monostate, MAA_VISION_NS::DirectHitParams, MAA_VISION_NS::TemplMatchingParams,
                            MAA_VISION_NS::OcrParams>;
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
    StartApp,
    StopApp,
    CustomTask,
    // InputText, // TODO
};

enum class Target
{
    Invalid = 0,
    Self,
    PreTask,
    Region,
};

using TargetParam = std::variant<std::monostate, std::string, cv::Rect>;

struct ClickParams
{
    Target target = Target::Self;
    TargetParam target_param;
};

struct SwipeParams
{
    Target begin = Target::Self;
    TargetParam begin_param;
    Target end = Target::Self;
    TargetParam end_param;

    uint duration = 200;
};

struct KeyParams
{
    std::vector<int> keys;
};

struct AppInfo
{
    std::string package;
};

struct CustomTaskParams
{
    std::string task_name;
    json::value task_param;
};

using Params = std::variant<std::monostate, ClickParams, SwipeParams, KeyParams, AppInfo, CustomTaskParams>;
} // namespace Action

struct WaitFreezesParams
{
    std::chrono::milliseconds time = std::chrono::milliseconds(0);

    Action::Target target = Action::Target::Self;
    Action::TargetParam target_param;

    double threshold = 0.95;
    int method = MAA_VISION_NS::TemplMatchingParams::kDefaultMethod;
};

struct TaskData
{
    std::string name;
    bool is_sub = false;

    Recognition::Type rec_type = Recognition::Type::DirectHit;
    Recognition::Params rec_params = MAA_VISION_NS::DirectHitParams {};

    bool cache = false;

    Action::Type action_type = Action::Type::DoNothing;
    Action::Params action_params;
    std::vector<std::string> next;

    std::chrono::milliseconds timeout = std::chrono::milliseconds(20 * 1000);
    std::vector<std::string> timeout_next;

    uint times_limit = UINT_MAX;
    std::vector<std::string> runout_next;

    std::chrono::milliseconds pre_delay = std::chrono::milliseconds(200);
    std::chrono::milliseconds post_delay = std::chrono::milliseconds(500);

    WaitFreezesParams pre_wait_freezes;
    WaitFreezesParams post_wait_freezes;

    bool notify = false;
};

MAA_PIPELINE_RES_NS_END
