#pragma once

#include <stack>
#include <string_view>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Instance/InstanceInternalAPI.hpp"
#include "Resource/PipelineResMgr.h"
#include "Resource/PipelineTypes.h"
#include "Task/TaskDataMgr.h"

MAA_TASK_NS_BEGIN

class Context : public MaaContextAPI
{
public:
    explicit Context(InstanceInternalAPI* inst);
    virtual ~Context() override = default;

public: // from MaaContextAPI
    virtual MaaTaskId run_task(std::string task, std::string_view param) override;
    virtual MaaTaskId run_recognition(
        cv::Mat image,
        std::string task,
        std::string_view param,
        /*out*/ cv::Rect& box,
        /*out*/ std::string& detail) override;
    virtual MaaTaskId run_action(std::string task, std::string_view param, cv::Rect cur_box, std::string cur_detail) override;

    virtual MaaScheduler* instance() override { return dynamic_cast<MaaScheduler*>(inst_); }

private:
    InstanceInternalAPI* inst_ = nullptr;
};

MAA_TASK_NS_END
