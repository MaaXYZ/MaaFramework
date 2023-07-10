#include "CustomTask.h"

#include "MaaCustomTask.h"
#include "MaaUtils/Logger.hpp"

MAA_TASK_NS_BEGIN

bool CustomTask::run()
{
    LogFunc;

    if (!task_handle_ || !task_handle_->run) {
        LogError << "Invalid task handle" << VAR_VOIDP(task_handle_) << VAR_VOIDP(task_handle_->run);
        return false;
    }

    return task_handle_->run();
}

bool CustomTask::set_param(const json::value& param)
{
    LogFunc << VAR(param);

    if (!task_handle_ || !task_handle_->set_param) {
        LogError << "Invalid task handle" << VAR_VOIDP(task_handle_) << VAR_VOIDP(task_handle_->set_param);
    }

    return task_handle_->set_param(param.to_string().c_str());
}

MAA_TASK_NS_END
