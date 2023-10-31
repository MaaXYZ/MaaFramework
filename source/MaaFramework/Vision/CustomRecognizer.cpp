#include "CustomRecognizer.h"

#include "Utils/NoWarningCV.hpp"

#include "Buffer/ImageBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "MaaFramework/Task/MaaCustomRecognizer.h"
#include "Task/SyncContext.h"
#include "Utils/Logger.h"

MAA_VISION_NS_BEGIN

CustomRecognizer::CustomRecognizer(MaaCustomRecognizerHandle handle, MaaTransparentArg handle_arg,
                                   InstanceInternalAPI* inst)
    : VisionBase(), recognizer_(handle), recognizer_arg_(handle_arg), inst_(inst)
{}

CustomRecognizer::ResultsVec CustomRecognizer::analyze() const
{
    LogFunc << VAR_VOIDP(recognizer_) << VAR_VOIDP(recognizer_->analyze) << VAR(param_.custom_param);

    if (!recognizer_ || !recognizer_->analyze) {
        LogError << "Recognizer is null";
        return {};
    }

    auto start_time = std::chrono::steady_clock::now();

    /*in*/
    MAA_TASK_NS::SyncContext sync_ctx(inst_);
    ImageBuffer image_buffer;
    image_buffer.set(image_);
    std::string custom_param_str = param_.custom_param.to_string();

    /*out*/
    MaaRect maa_box { 0 };
    StringBuffer detail_buffer;

    bool ret = recognizer_->analyze(&sync_ctx, &image_buffer, name_.c_str(), custom_param_str.c_str(), recognizer_arg_,
                                    &maa_box, &detail_buffer);

    cv::Rect box { maa_box.x, maa_box.y, maa_box.width, maa_box.height };
    std::string detail(detail_buffer.data(), detail_buffer.size());

    auto cost = duration_since(start_time);
    LogDebug << VAR(ret) << VAR(box) << VAR(detail) << VAR(cost);

    if (!ret) {
        return {};
    }

    auto jdetail = json::parse(detail).value_or(detail);
    return {
        Result { .box = box, .detail = std::move(jdetail) },
    };
}

MAA_VISION_NS_END
