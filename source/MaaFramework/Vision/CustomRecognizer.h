#pragma once

#include <vector>

#include <meojson/json.hpp>

#include "Instance/InstanceInternalAPI.hpp"
#include "MaaFramework/MaaDef.h"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class CustomRecognizer : public VisionBase
{
    struct Result
    {
        cv::Rect box {};
        json::value detail;

        operator json::value() const
        {
            json::value root;
            root["box"] = json::array({ box.x, box.y, box.width, box.height });
            root["detail"] = detail;
            return root;
        }
    };
    using ResultsVec = std::vector<Result>;

public:
    CustomRecognizer(MaaCustomRecognizerHandle handle, MaaTransparentArg handle_arg, InstanceInternalAPI* inst);

    void set_param(CustomRecognizerParam param) { param_ = std::move(param); }
    ResultsVec analyze() const;

private:
    MaaCustomRecognizerHandle recognizer_ = nullptr;
    MaaTransparentArg recognizer_arg_ = nullptr;
    InstanceInternalAPI* inst_ = nullptr;

    CustomRecognizerParam param_;
};

MAA_VISION_NS_END
