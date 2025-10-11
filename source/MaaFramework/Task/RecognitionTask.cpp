#include "RecognitionTask.h"

#include "Controller/ControllerAgent.h"
#include "Global/GlobalOptionMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"
#include "Utils/JsonExt.hpp"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

bool RecognitionTask::run()
{
    return run_with_param(screencap()) != MaaInvalidId;
}

MaaRecoId RecognitionTask::run_with_param(const cv::Mat& image)
{
    LogFunc << VAR(entry_);

    PipelineData::NextList next_list = { entry_ };

    std::chrono::milliseconds notify_cost; // 这玩意其实理论上应该能通过context自动添加到custom所属的cost里面? 不知道咋搞了
    auto reco = run_recognition(image, next_list, notify_cost);
    if (!reco.box) {
        LogDebug << "No reco result";
        return MaaInvalidId;
    }

    NodeDetail result {
        .node_id = generate_node_id(),
        .name = reco.name,
        .reco_id = reco.reco_id,
    };
    set_node_detail(result.node_id, result);

    return reco.reco_id;
}

MAA_TASK_NS_END
