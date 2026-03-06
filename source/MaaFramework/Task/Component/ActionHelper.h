#pragma once

#include "Common/Conf.h"
#include "Common/MaaTypes.h"
#include "Controller/ControllerAgent.h"
#include "Resource/PipelineTypes.h"

MAA_TASK_NS_BEGIN

class Tasker;

class ActionHelper : public NonCopyable
{
public:
    explicit ActionHelper(MaaContext* context);

    bool wait_freezes(const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& box, const std::string& name = "");

    cv::Rect get_target_rect(const MAA_RES_NS::Action::Target& target, const cv::Rect& box = { });

private:
    cv::Rect get_rect_from_node(const std::string& node_name) const;
    Tasker* tasker() const;
    MAA_CTRL_NS::ControllerAgent* controller();

private:
    MaaContext* context_ = nullptr;
};

MAA_TASK_NS_END
