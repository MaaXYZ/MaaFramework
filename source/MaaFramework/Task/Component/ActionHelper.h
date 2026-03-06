#pragma once

#include "Common/Conf.h"
#include "Common/MaaTypes.h"
#include "Controller/ControllerAgent.h"
#include "Resource/PipelineTypes.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

class ActionHelper : public NonCopyable
{
public:
    ActionHelper(Tasker* tasker, MaaContext* context);

    bool wait_freezes(const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& box, const std::string& name = "");

    cv::Rect get_target_rect(const MAA_RES_NS::Action::Target& target, const cv::Rect& box = { });

private:
    std::string resolve_anchor(const std::string& anchor_name) const;
    MAA_CTRL_NS::ControllerAgent* controller();

private:
    Tasker* tasker_ = nullptr;
    MaaContext* context_ = nullptr;
};

MAA_TASK_NS_END
