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
        json::value detail;

        json::value to_json() const
        {
            json::value root;
            root["box"] = json::array({ box.x, box.y, box.width, box.height });
            root["detail"] = detail;
            return root;
        }
    };
    using ResultOpt = std::optional<Result>;

public:
    CustomRecognizer(MaaCustomRecognizerHandle handle, InstanceInternalAPI* inst);

    void set_param(CustomParam param) { param_ = std::move(param); }
    ResultOpt analyze() const;

private:
    MaaCustomRecognizerHandle recognizer_ = nullptr;
    InstanceInternalAPI* inst_ = nullptr;

    CustomParam param_;
};

MAA_VISION_NS_END
