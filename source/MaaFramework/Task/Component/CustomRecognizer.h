#pragma once

#include <vector>

#include "API/MaaTypes.h"
#include "MaaFramework/MaaDef.h"
#include "Resource/ResourceMgr.h"
#include "Task/Context.h"
#include "Vision/VisionBase.h"

MAA_TASK_NS_BEGIN

struct CustomRecognizerResult
{
    cv::Rect box {};
    json::value detail;

    MEO_JSONIZATION(box, detail);
};

class CustomRecognizer
    : public MAA_VISION_NS::VisionBase
    , public MAA_VISION_NS::RecoResultAPI<CustomRecognizerResult>
{
public:
    CustomRecognizer(
        std::string name,
        MAA_RES_NS::CustomRecognizerSession session,
        Context& context,
        const MAA_VISION_NS::CustomRecognizerParam& param,
        const cv::Mat& image);

private:
    void analyze();

private:
    std::string name_;
    MAA_RES_NS::CustomRecognizerSession session_;

    Context& context_;
    const MAA_VISION_NS::CustomRecognizerParam& param_;
};

MAA_TASK_NS_END
