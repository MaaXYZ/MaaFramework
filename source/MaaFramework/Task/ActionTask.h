#pragma once

#include "MaaFramework/MaaDef.h"
#include "TaskBase.h"

#include "Common/Conf.h"

MAA_TASK_NS_BEGIN

class ActionTask : public TaskBase
{
public:
    using TaskBase::TaskBase;

    virtual ~ActionTask() override = default;

public:
    MaaActId run_with_param(const cv::Rect& box, const json::value& reco_detail);
};

MAA_TASK_NS_END
