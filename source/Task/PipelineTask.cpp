#include "PipelineTask.h"

MAA_TASK_NS_BEGIN

bool PipelineTask::run()
{
    return false;
}

bool PipelineTask::set_param(const json::value& param)
{
    return true;
}

MAA_TASK_NS_END
