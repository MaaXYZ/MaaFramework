#include "CustomRecognizer.h"

#include "MaaUtils/Logger.hpp"

MAA_VISION_NS_BEGIN

CustomRecognizer::ResultOpt CustomRecognizer::analyze() const
{
    LogFunc << VAR_VOIDP(recognizer_) << VAR(recognizer_->analyze) << VAR(param_.custom_param);

    if (!recognizer_ || !recognizer_->analyze) {
        LogError << "Recognizer is null";
        return std::nullopt;
    }

    MaaImage image {
        .rows = image_.rows, .cols = image_.cols, .type = image_.type(), .data = static_cast<void*>(image_.data)
    };

    auto result = recognizer_->analyze(image, param_.custom_param.to_string().c_str());

    cv::Rect box { result.box.x, result.box.y, result.box.width, result.box.height };
    LogTrace << VAR(result.success) << VAR(result.detail);

    if (!result.success) {
        return std::nullopt;
    }

    return Result { .box = box, .detail = result.detail };
}

MAA_VISION_NS_END
