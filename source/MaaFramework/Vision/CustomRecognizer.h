#pragma once

#include <vector>

#include "API/MaaTypes.h"
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
        Tasker* tasker,
        std::string name = "");

private:
    void analyze();

private:
    const CustomRecognizerParam param_;
    CustomRecognizerSession session_;
    Tasker* tasker_ = nullptr;
};

MAA_VISION_NS_END
