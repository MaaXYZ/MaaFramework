#include "RecognitionTask.h"

#include "Controller/ControllerAgent.h"
#include "Global/GlobalOptionMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"
#include "Utils/JsonExt.hpp"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

RecognitionTask::RecognitionTask(std::string entry, Tasker* tasker, PipelineDataMap pp_override)
    : TaskBase(std::move(entry), tasker, std::move(pp_override))
{
}

bool RecognitionTask::run()
{
    return run_with_param(screencap());
}

bool RecognitionTask::run_with_param(const cv::Mat& image)
{
    LogFunc << VAR(entry_);

    PipelineData::NextList next_list = { PipelineData::NextObject { .name = entry_ } };
    HitDetail hit_detail;

    auto iter = run_recogintion(image, next_list, hit_detail);
    if (iter == next_list.cend()) {
        return false;
    }

    hit_detail.pipeline_data = {};  // for do nothing
    run_action(hit_detail);

    // recognized
    return true;
}

MAA_TASK_NS_END
