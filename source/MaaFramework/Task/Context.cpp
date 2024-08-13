#include "Context.h"

#include <meojson/json.hpp>

#include "Utils/Logger.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

Context::Context(Tasker* tasker)
    : tasker_(tasker)
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

Tasker* Context::tasker()
{
    return tasker_;
}

MAA_TASK_NS_END
