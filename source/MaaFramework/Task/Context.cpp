#include "Context.h"

#include <meojson/json.hpp>

#include "Tasker/Tasker.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

Context::Context(TaskBase& task)
    : task_(task)
{
}

MaaTaskId Context::run_pipeline(std::string task, std::string_view param)
{
    return MaaTaskId();
}

MaaTaskId Context::run_recognition(std::string task, std::string_view param, cv::Mat image)
{
    return MaaTaskId();
}

MaaTaskId Context::run_action(std::string task, std::string_view param, cv::Rect cur_box, std::string cur_detail)
{
    return MaaTaskId();
}

MaaTaskId Context::task_id() const
{
    return task_.task_id();
}

Tasker* Context::tasker()
{
    return task_.tasker();
}

MAA_TASK_NS_END
