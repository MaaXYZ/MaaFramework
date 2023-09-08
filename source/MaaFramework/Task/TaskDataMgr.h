#pragma once

#include <string_view>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "Instance/InstanceInternalAPI.hpp"
#include "Resource/PipelineResMgr.h"
#include "Resource/PipelineTypes.h"

#include <stack>

MAA_TASK_NS_BEGIN

class TaskDataMgr
{
public:
    using TaskData = MAA_RES_NS::TaskData;

public:
    TaskDataMgr(InstanceInternalAPI* inst);

    const TaskData& get_task_data(const std::string& task_name);
    bool set_param(const json::value& param);

private:
    using TaskDataMap = MAA_RES_NS::PipelineResMgr::TaskDataMap;

    bool set_diff_task(const json::value& input);

private:
    MAA_RES_NS::ResourceMgr* resource() { return inst_ ? inst_->inter_resource() : nullptr; }

private:
    InstanceInternalAPI* inst_ = nullptr;
    TaskDataMap diff_tasks_;
};

MAA_TASK_NS_END
