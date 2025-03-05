#pragma once

#include <vector>

#include "Common/MaaTypes.h"
#include "MaaFramework/MaaDef.h"
#include "Resource/ResourceMgr.h"
#include "Task/Context.h"
#include "Vision/VisionBase.h"

MAA_TASK_NS_BEGIN

struct CustomRecognitionResult
{
    cv::Rect box {};
    json::value detail;

    MEO_JSONIZATION(box, detail);
};

class CustomRecognition
    : public MAA_VISION_NS::VisionBase
    , public MAA_VISION_NS::RecoResultAPI<CustomRecognitionResult>
{
public:
    CustomRecognition(
        const cv::Mat& image,
        const cv::Rect& roi,
        const MAA_VISION_NS::CustomRecognitionParam& param,
        MAA_RES_NS::CustomRecognitionSession session,
        Context& context,
        std::string name);

private:
    void analyze();

private:
    const MAA_VISION_NS::CustomRecognitionParam& param_;
    MAA_RES_NS::CustomRecognitionSession session_;
    Context& context_;
};

MAA_TASK_NS_END
