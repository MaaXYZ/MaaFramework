#pragma once

#include <optional>

#include <meojson/json.hpp>

#include "MaaFramework/MaaCustomRecognizer.h"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class CustomRecognizer : public VisionBase
{
    struct Result
    {
        cv::Rect box;
        std::string detail;
    };
    using ResultOpt = std::optional<Result>;

public:
    CustomRecognizer(MaaCustomRecognizerHandle handle) : VisionBase(nullptr), recognizer_(handle) {}

    void set_param(CustomParam param) { param_ = std::move(param); }
    ResultOpt analyze() const;

private:
    MaaCustomRecognizerHandle recognizer_ = nullptr;
    CustomParam param_;
};

MAA_VISION_NS_END
