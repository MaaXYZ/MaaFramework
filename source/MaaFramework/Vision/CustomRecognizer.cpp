#include "CustomRecognizer.h"

#include "Utils/NoWarningCV.hpp"

#include "Buffer/ImageBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "MaaFramework/Task/MaaCustomRecognizer.h"
#include "Task/SyncContext.h"
#include "Utils/Logger.h"

MAA_VISION_NS_BEGIN

CustomRecognizer::CustomRecognizer(
    cv::Mat image,
    CustomRecognizerParam param,
    CustomRecognizerSession session,
    InstanceInternalAPI* inst,
    std::string name)
    : VisionBase(std::move(image), std::move(name))
    , param_(std::move(param))
    , session_(std::move(session))
    , inst_(inst)
{
    analyze();
}

void CustomRecognizer::analyze()
{
    LogFunc << VAR_VOIDP(session_.recognizer) << VAR(param_.custom_param);

    if (!session_.recognizer || !session_.recognizer->analyze) {
        LogError << "Recognizer is nullptr";
        return;
    }

    auto start_time = std::chrono::steady_clock::now();

    /*in*/
    MAA_TASK_NS::SyncContext sync_ctx(inst_);
    ImageBuffer image_buffer(image_);
    std::string custom_param_str = param_.custom_param.to_string();

    /*out*/
    MaaRect maa_box { 0 };
    StringBuffer detail_buffer;

    bool ret = session_.recognizer->analyze(
        &sync_ctx,
        &image_buffer,
        name_.c_str(),
        custom_param_str.c_str(),
        session_.recognizer_arg,
        &maa_box,
        &detail_buffer);

    cv::Rect box { maa_box.x, maa_box.y, maa_box.width, maa_box.height };
    std::string detail(detail_buffer.data(), detail_buffer.size());

    auto jdetail = json::parse(detail).value_or(detail);
    Result res { .box = box, .detail = std::move(jdetail) };

    all_results_ = { res };
    if (ret) {
        filtered_results_ = { res };
        best_result_ = res;
    }

    auto cost = duration_since(start_time);
    LogTrace << name_ << VAR(uid_) << VAR(all_results_) << VAR(filtered_results_)
             << VAR(best_result_) << VAR(cost);
}

MAA_VISION_NS_END