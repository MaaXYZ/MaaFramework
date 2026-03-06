#include "ActionHelper.h"

#include "Common/TaskResultTypes.h"
#include "MaaUtils/Logger.h"
#include "Tasker/Tasker.h"
#include "Vision/TemplateComparator.h"
#include "Vision/VisionUtils.hpp"

MAA_TASK_NS_BEGIN

ActionHelper::ActionHelper(MaaContext* context)
    : context_(context)
{
}

bool ActionHelper::wait_freezes(const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& box, const std::string& name)
{
    if (param.time <= std::chrono::milliseconds(0)) {
        return true;
    }

    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    using namespace MAA_VISION_NS;

    LogTrace << "Wait freezes:" << VAR(param.time) << VAR(param.rate_limit) << VAR(param.timeout) << VAR(param.threshold)
             << VAR(param.method);

    auto rate_limit = std::min(param.rate_limit, param.time);

    auto screencap_clock = std::chrono::steady_clock::now();
    cv::Mat pre_image = controller()->screencap();

    cv::Rect roi = box;
    if (roi.empty() && !pre_image.empty()) {
        roi = cv::Rect(0, 0, pre_image.cols, pre_image.rows);
    }

    TemplateComparatorParam comp_param {
        .threshold = param.threshold,
        .method = param.method,
    };

    const auto start_clock = std::chrono::steady_clock::now();
    auto pre_image_clock = start_clock;

    while (true) {
        LogDebug << "sleep_until" << VAR(rate_limit);
        std::this_thread::sleep_until(screencap_clock + rate_limit);

        // timeout < 0 表示无限等待
        if (param.timeout >= std::chrono::milliseconds(0) && duration_since(start_clock) > param.timeout) {
            LogWarn << "Wait freezes timeout" << VAR(duration_since(start_clock)) << VAR(param.timeout);
            return false;
        }

        screencap_clock = std::chrono::steady_clock::now();
        cv::Mat cur_image = controller()->screencap();

        if (pre_image.empty() || cur_image.empty()) {
            LogError << "Image is empty" << VAR(pre_image.empty()) << VAR(cur_image.empty());
            return false;
        }

        std::string draw_name = name.empty() ? "wait_freezes" : std::format("{}_wait_freezes", name);
        TemplateComparator comparator(pre_image, cur_image, { roi }, comp_param, draw_name);

        VisionBase::save_draws(draw_name, comparator.draws());

        if (!comparator.best_result()) {
            pre_image = cur_image;
            pre_image_clock = std::chrono::steady_clock::now();
            continue;
        }

        if (duration_since(pre_image_clock) > param.time) {
            break;
        }
    }

    return true;
}

cv::Rect ActionHelper::get_target_rect(const MAA_RES_NS::Action::Target& target, const cv::Rect& box)
{
    if (!tasker()) {
        LogError << "Tasker is null";
        return { };
    }

    using namespace MAA_RES_NS::Action;

    cv::Rect raw { };
    switch (target.type) {
    case Target::Type::Self:
        raw = box;
        break;

    case Target::Type::PreTask: {
        const auto& name = std::get<std::string>(target.param);
        raw = get_rect_from_node(name);
        LogDebug << "pre task" << VAR(name) << VAR(raw);
    } break;

    case Target::Type::Anchor: {
        const auto& anchor_name = std::get<std::string>(target.param);
        auto node_name = context_->get_anchor(anchor_name);
        if (!node_name) {
            LogWarn << "anchor not set" << VAR(anchor_name);
            return { };
        }
        raw = get_rect_from_node(*node_name);
        LogDebug << "anchor" << VAR(anchor_name) << VAR(*node_name) << VAR(raw);
    } break;

    case Target::Type::Region:
        raw = std::get<cv::Rect>(target.param);
        break;

    default:
        LogError << "Unknown target type" << VAR(static_cast<int>(target.type));
        return { };
    }

    // 无 controller 时跳过边界检查，直接返回 raw + offset
    if (!controller()) {
        LogDebug << "controller not bound, skip image boundary check";
        return cv::Rect(
            raw.x + target.offset.x,
            raw.y + target.offset.y,
            raw.width + target.offset.width,
            raw.height + target.offset.height);
    }

    auto image = controller()->cached_image();

    // Region 类型支持负数坐标和尺寸
    if (target.type == Target::Type::Region) {
        raw = MAA_VISION_NS::normalize_rect(raw, image.cols, image.rows);
    }

    int x = std::clamp(raw.x + target.offset.x, 0, image.cols);
    int y = std::clamp(raw.y + target.offset.y, 0, image.rows);
    int width = std::clamp(raw.width + target.offset.width, 0, image.cols - x);
    int height = std::clamp(raw.height + target.offset.height, 0, image.rows - y);

    return cv::Rect(x, y, width, height);
}

cv::Rect ActionHelper::get_rect_from_node(const std::string& node_name) const
{
    auto* t = tasker();
    if (!t) {
        return { };
    }

    auto& cache = t->runtime_cache();
    auto node_id = cache.get_latest_node(node_name);
    if (!node_id) {
        LogWarn << "node not found or not executed" << VAR(node_name);
        return { };
    }
    NodeDetail node_detail = cache.get_node_detail(*node_id).value_or(NodeDetail { });
    RecoResult reco_result = cache.get_reco_result(node_detail.reco_id).value_or(RecoResult { });
    return reco_result.box.value_or(cv::Rect { });
}

Tasker* ActionHelper::tasker() const
{
    return context_ ? static_cast<Tasker*>(context_->tasker()) : nullptr;
}

MAA_CTRL_NS::ControllerAgent* ActionHelper::controller()
{
    auto* t = tasker();
    return t ? t->controller() : nullptr;
}

MAA_TASK_NS_END
