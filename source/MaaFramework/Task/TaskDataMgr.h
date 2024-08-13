#pragma once

#include <stack>
#include <string_view>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Resource/PipelineResMgr.h"
#include "Resource/PipelineTypes.h"

MAA_TASK_NS_BEGIN

class TaskDataMgr
{
public:
    using TaskData = MAA_RES_NS::TaskData;
    using TaskDataMap = MAA_RES_NS::PipelineResMgr::TaskDataMap;

public:
    explicit TaskDataMgr(Tasker* tasker);

    const TaskData& get_task_data(const std::string& task_name);
    bool override_pipeline(const json::value& pipeline_override);

private:
    MAA_RES_NS::ResourceMgr* resource() { return tasker_ ? tasker_->resource() : nullptr; }

private:
    Tasker* tasker_ = nullptr;
    TaskDataMap pipeline_override_;
};

MAA_TASK_NS_END
