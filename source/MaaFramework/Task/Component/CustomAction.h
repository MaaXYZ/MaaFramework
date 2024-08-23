#pragma once

#include "Conf/Conf.h"

#include "API/MaaTypes.h"
#include "MaaFramework/MaaDef.h"
#include "Resource/ResourceMgr.h"
#include "Task/Context.h"

MAA_TASK_NS_BEGIN

class CustomAction
{
public:
    CustomAction(std::string name, MAA_RES_NS::CustomActionSession session);
    ~CustomAction() = default;

public:
    bool run(Context& context, const MAA_RES_NS::Action::CustomParam& param, const cv::Rect& box, const json::value& reco_detail);

private:
    std::string name_;
    MAA_RES_NS::CustomActionSession session_;
};

MAA_TASK_NS_END
