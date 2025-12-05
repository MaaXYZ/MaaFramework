#pragma once

#include "MaaFramework/MaaDef.h"
#include "TaskBase.h"

#include "Common/Conf.h"

MAA_TASK_NS_BEGIN

class ActionTask : public TaskBase
{
public:
    ActionTask(
        const cv::Rect& box,
        const std::string& reco_detail,
        std::string entry,
        Tasker* tasker,
        std::shared_ptr<Context> context = nullptr);

    virtual ~ActionTask() override = default;

public:
    virtual bool run() override;

public:
    MaaActId run_impl();

private:
    cv::Rect box_;
    json::value reco_detail_;
};

MAA_TASK_NS_END
