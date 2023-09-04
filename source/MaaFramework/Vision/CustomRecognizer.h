#pragma once

#include <vector>

#include <meojson/json.hpp>

#include "MaaFramework/MaaCustomRecognizer.h"
#include "VisionBase.h"
#include "VisionTypes.h"
#include "Task/TaskInstAPI.h"

MAA_VISION_NS_BEGIN

class CustomRecognizer : public VisionBase
{
    struct Result
    {
        cv::Rect box {};
        json::value detail;

        json::value to_json() const
        {
            json::value root;
            root["box"] = json::array({ box.x, box.y, box.width, box.height });
            root["detail"] = detail;
            return root;
        }
    };
    using ResultsVec = std::vector<Result>;

public:
    CustomRecognizer(MaaCustomRecognizerHandle handle, InstanceInternalAPI* inst);

    void set_task_inst(MAA_TASK_NS::TaskInstAPI* task_inst) { task_inst_ = task_inst; }
    void set_param(CustomParam param) { param_ = std::move(param); }
    ResultsVec analyze() const;

private:
    MaaCustomRecognizerHandle recognizer_ = nullptr;
    InstanceInternalAPI* inst_ = nullptr;

    MAA_TASK_NS::TaskInstAPI* task_inst_ = nullptr;
    CustomParam param_;
};

MAA_VISION_NS_END
