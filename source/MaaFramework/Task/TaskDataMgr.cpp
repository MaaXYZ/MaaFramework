#include "TaskDataMgr.h"

#include "Resource/ResourceMgr.h"
#include "Utils/ImageIo.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

TaskDataMgr::TaskDataMgr(Tasker* tasker)
    : inst_(inst)
{
}

const MAA_RES_NS::TaskData& TaskDataMgr::get_task_data(const std::string& task_name)
{
    if (!resource()) {
        LogError << "Resource not binded";
        static MAA_RES_NS::TaskData empty;
        return empty;
    }
    auto& raw_data_mgr = resource()->pipeline_res();

    auto diff_it = pipeline_override_.find(task_name);
    if (diff_it == pipeline_override_.end()) {
        return raw_data_mgr.get_task_data(task_name);
    }

    return diff_it->second;
}

bool TaskDataMgr::override_pipeline(const json::value& pipeline_override)
{
    LogFunc << VAR(pipeline_override);

    if (!resource()) {
        LogError << "Resource not binded";
        return false;
    }

    MAA_RES_NS::PipelineResMgr::TaskDataMap new_override;
    auto& raw_data_map = resource()->pipeline_res().get_task_data_map();
    std::set<std::string> existing_keys;
    bool parsed = MAA_RES_NS::PipelineResMgr::parse_config(pipeline_override, new_override, existing_keys, raw_data_map);
    if (!parsed) {
        LogError << "Parse json failed";
        return false;
    }

    new_override.merge(std::move(pipeline_override_));
    pipeline_override_ = std::move(new_override);

    return true;
}

MAA_TASK_NS_END
