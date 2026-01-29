#pragma once

#include "Common/Conf.h"
#include "Controller/ControllerAgent.h"
#include "Resource/PipelineTypes.h"

MAA_TASK_NS_BEGIN

class Tasker;

// 等待画面静止的工具类，可被 Actuator 和 Context 复用
class WaitFreezes
{
public:
    // 执行等待画面静止操作
    // controller: 控制器，用于截图
    // param: 等待参数
    // box: ROI 区域，如果为空则使用整个屏幕
    static bool wait(MAA_CTRL_NS::ControllerAgent* controller, const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& box);

    // 从 tasker 获取 controller 并执行等待
    // tasker: 任务器
    // param: 等待参数
    // box: ROI 区域，如果为空则使用整个屏幕
    static bool wait(Tasker* tasker, const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& box);
};

MAA_TASK_NS_END
