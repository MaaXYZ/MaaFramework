#include "WaitFreezes.h"

#include "MaaUtils/Logger.h"
#include "Tasker/Tasker.h"
#include "Vision/TemplateComparator.h"

MAA_TASK_NS_BEGIN

bool WaitFreezes::wait(MAA_CTRL_NS::ControllerAgent* controller, const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& box)
{
    if (param.time <= std::chrono::milliseconds(0)) {
        return true;
    }

    if (!controller) {
        LogError << "Controller is null";
        return false;
    }

    using namespace MAA_VISION_NS;

    LogTrace << "Wait freezes:" << VAR(param.time) << VAR(param.rate_limit) << VAR(param.timeout) << VAR(param.threshold)
             << VAR(param.method);

    auto rate_limit = std::min(param.rate_limit, param.time);

    auto screencap_clock = std::chrono::steady_clock::now();
    cv::Mat pre_image = controller->screencap();

    // 使用传入的 box 作为 ROI，如果 box 为空则使用整个屏幕
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

        // timeout < 0 表示无限等待，跳过超时检查
        if (param.timeout >= std::chrono::milliseconds(0) && duration_since(start_clock) > param.timeout) {
            LogWarn << "Wait freezes timeout" << VAR(duration_since(start_clock)) << VAR(param.timeout);
            return false;
        }

        screencap_clock = std::chrono::steady_clock::now();
        cv::Mat cur_image = controller->screencap();

        if (pre_image.empty() || cur_image.empty()) {
            LogError << "Image is empty" << VAR(pre_image.empty()) << VAR(cur_image.empty());
            return false;
        }

        TemplateComparator comparator(pre_image, cur_image, { roi }, comp_param);

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

bool WaitFreezes::wait(Tasker* tasker, const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& box)
{
    if (!tasker) {
        LogError << "Tasker is null";
        return false;
    }

    auto* controller = tasker->controller();
    return wait(controller, param, box);
}

MAA_TASK_NS_END
