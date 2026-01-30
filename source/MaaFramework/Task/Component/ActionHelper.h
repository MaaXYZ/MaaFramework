#pragma once

#include "Common/Conf.h"
#include "Controller/ControllerAgent.h"
#include "Resource/PipelineTypes.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

class ActionHelper : public NonCopyable
{
public:
    explicit ActionHelper(Tasker* tasker);

    // 等待画面静止
    bool wait_freezes(const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& box);

    // 根据 target 计算 ROI 区域
    cv::Rect get_target_rect(const MAA_RES_NS::Action::Target& target, const cv::Rect& box = {});

private:
    MAA_CTRL_NS::ControllerAgent* controller();

private:
    Tasker* tasker_ = nullptr;
};

MAA_TASK_NS_END
