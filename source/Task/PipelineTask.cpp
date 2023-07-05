#include "PipelineTask.h"

#include "Controller/ControllerMgr.h"
#include "MaaUtils/Logger.hpp"
#include "Resource/ResourceMgr.h"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"
#include "Vision/VisionUtils.hpp"

MAA_TASK_NS_BEGIN

bool PipelineTask::run()
{
    LogFunc << VAR(first_task_);

    const auto& data_mgr = resource()->pipeline_cfg();
    auto cur_task = data_mgr.get_data(first_task_);
    std::vector<std::string> next_list = { first_task_ };

    RunningResult ret = RunningResult::Success;
    while (!next_list.empty() && !need_exit()) {
        ret = find_first_and_run(next_list, cur_task.timeout, cur_task);

        switch (ret) {
        case RunningResult::Success:
            next_list = cur_task.next;
            break;
        case RunningResult::Timeout:
            next_list = cur_task.timeout_next;
            break;
        case RunningResult::Runout:
            next_list = cur_task.runout_next;
            break;
        case RunningResult::Interrupted:
            next_list.clear();
            break;
        default:
            break;
        }
    }

    return ret == RunningResult::Success;
}

bool PipelineTask::set_param(const json::value& param)
{
    LogFunc << VAR(param);

    return true;
}

PipelineTask::RunningResult PipelineTask::find_first_and_run(const std::vector<std::string>& list,
                                                             std::chrono::milliseconds find_timeout,
                                                             /*out*/ MAA_PIPELINE_RES_NS::TaskData& found_data)
{
    FindResult result;

    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        auto find_opt = find_first(list);
        if (find_opt) {
            result = *std::move(find_opt);
            break;
        }

        if (std::chrono::steady_clock::now() - start_time > find_timeout) {
            return RunningResult::Timeout;
        }
        if (need_exit()) {
            return RunningResult::Interrupted;
        }
    }

    if (need_exit()) {
        return RunningResult::Interrupted;
    }

    uint64_t run_times = status()->get_pipeline_run_times(result.task_data.name);
    if (result.task_data.times_limit <= run_times) {
        found_data = std::move(result.task_data);
        return RunningResult::Runout;
    }

    start_to_act(result);

    status()->increase_pipeline_run_times(result.task_data.name);

    found_data = std::move(result.task_data);
    return RunningResult::Success;
}

std::optional<PipelineTask::FindResult> PipelineTask::find_first(const std::vector<std::string>& list)
{
    const auto& data_mgr = resource()->pipeline_cfg();

    for (const std::string& name : list) {
        const auto& task_data = data_mgr.get_data(name);
        auto rec_opt = recognize(task_data.rec_type, task_data.rec_params);
        if (!rec_opt) {
            continue;
        }
        return FindResult { .rec = *std::move(rec_opt), .task_data = task_data };
    }
    return std::nullopt;
}

std::optional<PipelineTask::RecResult> PipelineTask::recognize(MAA_PIPELINE_RES_NS::Recognition::Type type,
                                                               const MAA_PIPELINE_RES_NS::Recognition::Params& param)
{
    using namespace MAA_PIPELINE_RES_NS::Recognition;
    using namespace MAA_VISION_NS;

    cv::Mat image = controller()->screencap();

    switch (type) {
    case Type::DirectHit:
        return direct_hit(image, std::get<DirectHitParams>(param));
    case Type::TemplateMatch:
        return template_match(image, std::get<TemplMatchingParams>(param));
    case Type::OcrDetAndRec:
        return ocr_det_and_rec(image, std::get<OcrParams>(param));
    case Type::OcrOnlyRec:
        return ocr_only_rec(image, std::get<OcrParams>(param));
    case Type::FreezesWait:
        return freezes_wait(image, std::get<FreezesWaitingParams>(param));
    default:
        LogError << "Unknown type" << VAR(static_cast<int>(type));
        return std::nullopt;
    }
}

std::optional<PipelineTask::RecResult> PipelineTask::direct_hit(const cv::Mat& image,
                                                                const MAA_VISION_NS::DirectHitParams& param)
{
    std::ignore = image;

    return RecResult { .box = param.roi.empty() ? cv::Rect() : param.roi.front() };
}

std::optional<PipelineTask::RecResult> PipelineTask::template_match(const cv::Mat& image,
                                                                    const MAA_VISION_NS::TemplMatchingParams& param)
{
    using namespace MAA_VISION_NS;

    Matcher matcher(inst(), image);
    matcher.set_param(param);

    auto ret = matcher.analyze();
    if (!ret) {
        return std::nullopt;
    }
    return RecResult { .box = ret->box };
}

std::optional<PipelineTask::RecResult> PipelineTask::ocr_det_and_rec(const cv::Mat& image,
                                                                     const MAA_VISION_NS::OcrParams& param)
{
    using namespace MAA_VISION_NS;

    OCRer ocer(inst(), image);
    ocer.set_param(param);

    auto ret = ocer.analyze();
    if (!ret) {
        return std::nullopt;
    }
    auto& res = *ret;

    // TODO: sort by required regex.
    // sort_by_required_(res, param.text);

    return RecResult { .box = res.front().box };
}

std::optional<PipelineTask::RecResult> PipelineTask::ocr_only_rec(const cv::Mat& image,
                                                                  const MAA_VISION_NS::OcrParams& param)
{
    std::ignore = image;
    std::ignore = param;

    return std::nullopt;
}

std::optional<PipelineTask::RecResult> PipelineTask::freezes_wait(const cv::Mat& image,
                                                                  const MAA_VISION_NS::FreezesWaitingParams& param)
{
    std::ignore = image;
    std::ignore = param;

    return std::nullopt;
}

void PipelineTask::start_to_act(const FindResult& act)
{
    sleep(act.task_data.pre_delay);

    // TODO: act

    sleep(act.task_data.post_delay);
}

MAA_TASK_NS_END
