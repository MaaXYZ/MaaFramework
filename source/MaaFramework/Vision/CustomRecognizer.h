#pragma once

#include <vector>

#include "Instance/InstanceInternalAPI.hpp"
#include "MaaFramework/MaaDef.h"
#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

struct CustomRecognizerResult
{
    cv::Rect box {};
    json::value detail;

    MEO_JSONIZATION(box, detail);
};

class CustomRecognizer
    : public VisionBase
    , public RecoResultAPI<CustomRecognizerResult>
{
public:
    CustomRecognizer(
        cv::Mat image,
        CustomRecognizerParam param,
        CustomRecognizerSession session,
        InstanceInternalAPI* inst,
        std::string name = "");

private:
    void analyze();

private:
    const CustomRecognizerParam param_;
    CustomRecognizerSession session_;
    InstanceInternalAPI* inst_ = nullptr;
};

MAA_VISION_NS_END
