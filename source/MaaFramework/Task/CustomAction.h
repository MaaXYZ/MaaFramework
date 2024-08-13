#pragma once

#include "Conf/Conf.h"

#include "API/MaaTypes.h"
#include "MaaFramework/MaaDef.h"
#include "Resource/PipelineTypes.h"
#include "Resource/ResourceMgr.h"
#include "Task/Context.h"

MAA_TASK_NS_BEGIN

class CustomAction
{
public:
    CustomAction(std::string name, MaaCustomActionCallback action, MaaTransparentArg trans_arg);
    ~CustomAction() = default;

public:
    bool run(Context& context, const MAA_RES_NS::Action::CustomParam& param, const cv::Rect& cur_box, const json::value& cur_rec_detail);

private:
    std::string name_;
    MaaCustomActionCallback action_callback_ = nullptr;
    MaaTransparentArg trans_arg_ = nullptr;
};

MAA_TASK_NS_END
