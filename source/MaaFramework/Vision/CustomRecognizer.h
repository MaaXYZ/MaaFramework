#pragma once

#include <vector>

#include "Instance/InstanceInternalAPI.hpp"
#include "MaaFramework/MaaDef.h"
#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class CustomRecognizer : public VisionBase
{
    struct Result
    {
        cv::Rect box {};
        json::value detail;

        MEO_JSONIZATION(box, detail);
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
