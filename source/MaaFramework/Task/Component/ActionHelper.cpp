#include "ActionHelper.h"

#include "Common/TaskResultTypes.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/Logger.h"
#include "Recognizer.h"
#include "Task/Context.h"
#include "Tasker/Tasker.h"
#include "Vision/TemplateComparator.h"
#include "Vision/VisionUtils.hpp"

MAA_TASK_NS_BEGIN

ActionHelper::ActionHelper(Context* context)
    : context_(context)
{
}

bool ActionHelper::wait_freezes(
    const MAA_RES_NS::WaitFreezesParam& param,
    const cv::Rect& ref_box,
    const WaitFreezesNotifyContext& noti_ctx)
{
    if (param.time <= std::chrono::milliseconds(0)) {
        return true;
    }

    auto roi = get_target_rect(param.target, ref_box);
    if (roi.empty()) {
        LogError << "failed to get target rect for wait_freezes" << VAR(noti_ctx.name);
        return false;
    }

    if (!controller()) {
        LogError << "Controller is null";
        return false;
    }

    using namespace MAA_VISION_NS;

    LogTrace << "Wait freezes:" << VAR(param.time) << VAR(param.rate_limit) << VAR(param.timeout) << VAR(param.threshold)
             << VAR(param.method);

    const MaaWfId wf_id = generate_wf_id();

    json::value cb_detail {
        { "task_id", context_ ? context_->task_id() : MaaInvalidId },
        { "wf_id", wf_id },
        { "name", noti_ctx.name },
        { "phase", noti_ctx.phase },
        { "roi", roi },
        { "param",
          {
              { "time", param.time.count() },
              { "threshold", param.threshold },
              { "method", param.method },
              { "rate_limit", param.rate_limit.count() },
              { "timeout", param.timeout.count() },
          } },
        { "focus", noti_ctx.focus },
    };
    notify(MaaMsg_Node_WaitFreezes_Starting, cb_detail);

    const auto start_clock = std::chrono::steady_clock::now();
    std::vector<MaaRecoId> reco_ids;

    auto finish = [&](bool success) {
        auto elapsed_ms = duration_since(start_clock).count();

        cb_detail["reco_ids"] = json::array(reco_ids);
        cb_detail["elapsed"] = elapsed_ms;
        notify(success ? MaaMsg_Node_WaitFreezes_Succeeded : MaaMsg_Node_WaitFreezes_Failed, cb_detail);

        if (auto* t = tasker()) {
            t->runtime_cache().set_wf_detail(
                wf_id,
                WaitFreezesDetail {
                    .wf_id = wf_id,
                    .name = noti_ctx.name,
                    .phase = noti_ctx.phase,
                    .success = success,
                    .elapsed_ms = elapsed_ms,
                    .reco_ids = reco_ids,
                    .roi = roi,
                });
        }

        return success;
    };

    auto rate_limit = std::min(param.rate_limit, param.time);

    auto screencap_clock = std::chrono::steady_clock::now();
    cv::Mat pre_image = controller()->screencap();

    auto corrected_roi = correct_roi(roi, pre_image);
    if (!corrected_roi) {
        LogError << "corrected roi is empty" << VAR(roi);
        return finish(false);
    }

    TemplateComparatorParam comp_param {
        .threshold = param.threshold,
        .method = param.method,
    };

    auto pre_image_clock = start_clock;

    while (true) {
        LogDebug << "sleep_until" << VAR(rate_limit);
        std::this_thread::sleep_until(screencap_clock + rate_limit);

        if (param.timeout >= std::chrono::milliseconds(0) && duration_since(start_clock) > param.timeout) {
            LogWarn << "Wait freezes timeout" << VAR(duration_since(start_clock)) << VAR(param.timeout);
            return finish(false);
        }

        screencap_clock = std::chrono::steady_clock::now();
        cv::Mat cur_image = controller()->screencap();

        if (pre_image.empty() || cur_image.empty()) {
            LogError << "Image is empty" << VAR(pre_image.empty()) << VAR(cur_image.empty());
            return finish(false);
        }

        std::string draw_name = noti_ctx.name.empty() ? "wait_freezes" : std::format("{}_wait_freezes", noti_ctx.name);
        TemplateComparator comparator(pre_image, cur_image, { *corrected_roi }, comp_param, draw_name);

        const MaaRecoId reco_id = Recognizer::generate_reco_id();
        RecoResult reco_result {
            .reco_id = reco_id,
            .name = draw_name,
            .algorithm = "WaitFreezes",
            .box = comparator.best_result() ? std::make_optional(comparator.best_result()->box) : std::nullopt,
            .detail =
                json::value {
                    { "all", json::array(comparator.all_results()) },
                    { "filtered", json::array(comparator.filtered_results()) },
                    { "best", comparator.best_result() ? json::value(*comparator.best_result()) : json::value(nullptr) },
                },
            .draws = comparator.draws(),
        };
        if (auto* t = tasker()) {
            t->runtime_cache().set_reco_detail(reco_id, std::move(reco_result));
        }
        reco_ids.emplace_back(reco_id);

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

    return finish(true);
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
        if (raw.empty()) {
            LogWarn << "pre task has no rect" << VAR(name);
            return { };
        }
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
        if (raw.empty()) {
            LogWarn << "anchor node has no rect" << VAR(anchor_name) << VAR(*node_name);
            return { };
        }
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
    return context_ ? context_->tasker() : nullptr;
}

void ActionHelper::notify(std::string_view msg, const json::value& detail)
{
    auto* t = tasker();
    if (!t || !context_) {
        return;
    }
    t->context_notify(context_, msg, detail);
}

MAA_CTRL_NS::ControllerAgent* ActionHelper::controller()
{
    auto* t = tasker();
    return t ? t->controller() : nullptr;
}

MAA_TASK_NS_END
