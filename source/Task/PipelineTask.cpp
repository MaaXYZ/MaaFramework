#include "PipelineTask.h"

#include "Utils/Logger.hpp"

MAA_TASK_NS_BEGIN

bool PipelineTask::run()
{
    LogFunc;

    return false;
}

bool PipelineTask::set_param(const json::value& param)
{
    LogFunc << VAR(param);

    return true;
}

MAA_TASK_NS_END
