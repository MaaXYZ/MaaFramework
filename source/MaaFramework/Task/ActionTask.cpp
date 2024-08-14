#include "PipelineTask.h"

#include "Controller/ControllerAgent.h"
#include "Global/GlobalOptionMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"
#include "Utils/JsonExt.hpp"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

PipelineTask::PipelineTask(std::string entry, Tasker* tasker, PipelineDataMap pp_override)
    : TaskBase(std::move(entry), tasker, std::move(pp_override))
{
}

bool PipelineTask::run()
{
    LogFunc << VAR(entry_);

    HitDetail fake_hit { .pipeline_data = context_.get_pipeline_data(entry_) };
    return run_action(fake_hit);
}

MAA_TASK_NS_END
