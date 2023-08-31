#include "CustomRecognizer.h"

#include "Utils/NoWarningCV.hpp"

#include "Buffer/ImageBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "Task/SyncContext.h"
#include "Utils/Logger.h"

MAA_VISION_NS_BEGIN

CustomRecognizer::CustomRecognizer(MaaCustomRecognizerHandle handle, InstanceInternalAPI* inst)
    : VisionBase(nullptr), recognizer_(handle), inst_(inst)
{}

CustomRecognizer::ResultOpt CustomRecognizer::analyze() const
{
    LogFunc << VAR_VOIDP(recognizer_) << VAR(recognizer_->analyze) << VAR(param_.custom_param);

    if (!recognizer_ || !recognizer_->analyze) {
        LogError << "Recognizer is null";
        return std::nullopt;
    }

    /*in*/
    MAA_TASK_NS::SyncContext sync_ctx(inst_);
    ImageBuffer image_buffer;
    image_buffer.set(image_);
    std::string custom_param_str = param_.custom_param.to_string();

    /*out*/
    MaaRect maa_box { 0 };
    StringBuffer detail_buffer;

    bool ret = recognizer_->analyze(&sync_ctx, &image_buffer, name_.c_str(), custom_param_str.c_str(), &maa_box,
                                    &detail_buffer);

    cv::Rect box { maa_box.x, maa_box.y, maa_box.width, maa_box.height };
    std::string detail(detail_buffer.data(), detail_buffer.size());
    LogDebug << VAR(ret) << VAR(box) << VAR(detail);

    if (!ret) {
        return std::nullopt;
    }

    auto jdetail = json::parse(detail).value_or(detail);

    return Result { .box = box, .detail = std::move(jdetail) };
}

MAA_VISION_NS_END
