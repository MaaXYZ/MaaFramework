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
    LogFunc << VAR(entry_);

    PipelineData::NextList next_list = { PipelineData::NextObject { .name = entry_ } };
    HitDetail hit_detail;
    auto iter = run_recogintion(next_list, hit_detail);
    bool hit = iter != next_list.cend();
    if (hit) {
        NodeDetail node { .hit = std::move(hit_detail), .status = NodeStatus::OnlyRecognized };
        auto nid = node.node_id;
        add_node_detail(nid, std::move(node));
    }

    return hit;
}

MAA_TASK_NS_END
