#include "TaskDataMgr.h"

#include "Resource/ResourceMgr.h"
#include "Utils/ImageIo.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

TaskDataMgr::TaskDataMgr(InstanceInternalAPI* inst) : inst_(inst) {}

const MAA_RES_NS::TaskData& TaskDataMgr::get_task_data(const std::string& task_name)
{
    if (!resource()) {
        LogError << "Resource not binded";
        static MAA_RES_NS::TaskData empty;
        return empty;
    }
    auto& raw_data_mgr = resource()->pipeline_res();

    auto diff_it = diff_tasks_.find(task_name);
    if (diff_it == diff_tasks_.end()) {
        return raw_data_mgr.get_task_data(task_name);
    }

    return diff_it->second;
}

bool TaskDataMgr::set_param(const json::value& param)
{
    LogFunc << VAR(param);

    bool ret = true;

    auto diff_opt = param.find<json::object>("diff_task");
    if (diff_opt) {
        ret &= set_diff_task(*diff_opt);
    }

    return ret;
}

bool TaskDataMgr::set_diff_task(const json::value& input)
{
    LogFunc << VAR(input);

    if (!resource()) {
        LogError << "Resource not binded";
        return false;
    }

    MAA_RES_NS::PipelineResMgr::TaskDataMap task_data_map;
    auto& raw_data_map = resource()->pipeline_res().get_task_data_map();
    std::set<std::string> existing_keys;
    bool parsed = MAA_RES_NS::PipelineResMgr::parse_config(input, task_data_map, existing_keys, raw_data_map);
    if (!parsed) {
        LogError << "Parse json failed";
        return false;
    }

    task_data_map.merge(std::move(diff_tasks_));
    diff_tasks_ = std::move(task_data_map);
    return true;
}

MAA_TASK_NS_END
