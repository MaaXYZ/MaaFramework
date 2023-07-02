#include "PipelineTask.h"

#include "MaaUtils/Logger.hpp"

MAA_TASK_NS_BEGIN

bool PipelineTask::run()
{
    LogFunc;

    return false;
}

bool PipelineTask::set_param(const json::value& param)
{
    LogFunc << VAR(param);

    auto begin_opt = param.find<std::string>("begin");
    if (!begin_opt) {
        LogError << "begin not found";
        return false;
    }
    begin_task_ = *begin_opt;
    connecting_task_ = param.get("connecting", "");
    disconnected_task_ = param.get("disconnected", "");

    LogInfo << VAR(begin_task_) << VAR(connecting_task_) << VAR(disconnected_task_);
    return true;
}

MAA_TASK_NS_END
