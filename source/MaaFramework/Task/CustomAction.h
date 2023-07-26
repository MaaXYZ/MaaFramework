#pragma once

#include "Conf/Conf.h"

#include "MaaFramework/MaaDef.h"
#include "Resource/PipelineTypes.h"

MAA_TASK_NS_BEGIN

class CustomAction
{
public:
    CustomAction(MaaCustomActionHandle handle);

    bool run(const MAA_PIPELINE_RES_NS::Action::CustomParam& param, const cv::Rect& cur_box,
             const json::value& rec_detail);

private:
    MaaCustomActionHandle action_ = nullptr;
};

MAA_TASK_NS_END
