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

    PipelineData::NextList next_list = { entry_ };

    auto reco = run_recogintion(image, next_list);
    if (!reco.box) {
        LogTrace << "No reco result";
        return false;
    }

    NodeDetail result {
        .node_id = generate_node_id(),
        .name = reco.name,
        .reco_id = reco.reco_id,
    };
    add_node_detail(result.node_id, result);

    return true;
}

MAA_TASK_NS_END
