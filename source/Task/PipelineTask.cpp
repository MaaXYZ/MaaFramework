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
    using namespace MAA_PIPELINE_RES_NS;

    LogFunc << VAR(task_name_);

    const TaskData& task_data = resource()->pipeline_cfg().get_data(std::string(task_name_));

    bool timeout = false;
    auto start_time = std::chrono::steady_clock::now();

    RecResult rec_result;
    while (!timeout && !need_exit()) {
        auto rec_opt = recognize(task_data.rec_type, task_data.rec_params);
        if (!rec_opt) {
            timeout = std::chrono::steady_clock::now() - start_time > std::chrono::milliseconds(task_data.timeout);
            continue;
        }
        rec_result = std::move(*rec_opt);
        break;
    }

    sleep(task_data.pre_delay);

    start_to_act(rec_result);

    sleep(task_data.post_delay);

    // TODO: switch to next.

    return true;
}

bool PipelineTask::set_param(const json::value& param)
{
    LogFunc << VAR(param);

    return true;
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

void PipelineTask::start_to_act(const RecResult& result)
{
    std::ignore = result;
}

MAA_TASK_NS_END
