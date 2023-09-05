#pragma once

#include "Conf/Conf.h"

#include "Instance/InstanceInternalAPI.hpp"
#include "MaaFramework/MaaDef.h"
#include "Resource/PipelineTypes.h"

MAA_TASK_NS_BEGIN

class CustomAction
{
public:
    CustomAction(MaaCustomActionHandle handle, InstanceInternalAPI* inst);

    bool run(const std::string& task_name, const MAA_RES_NS::Action::CustomParam& param, const cv::Rect& cur_box,
             const json::value& cur_rec_detail);

private:
    MaaCustomActionHandle action_ = nullptr;
    InstanceInternalAPI* inst_ = nullptr;
};

MAA_TASK_NS_END
