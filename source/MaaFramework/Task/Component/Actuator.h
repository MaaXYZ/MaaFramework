#pragma once

#include <stack>
#include <string_view>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Controller/ControllerAgent.h"
#include "Resource/PipelineTypes.h"
#include "Task/Context.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

class Actuator
{
public:
    using PipelineData = MAA_RES_NS::PipelineData;
    using PreTaskBoxes = std::map<std::string, cv::Rect>;

public:
    Actuator(Tasker* tasker, Context& context);

    bool run(const cv::Rect& reco_hit, MaaRecoId reco_id, const PipelineData& pipeline_data);

private:
    bool click(const MAA_RES_NS::Action::ClickParam& param, const cv::Rect& box);
    bool swipe(const MAA_RES_NS::Action::SwipeParam& param, const cv::Rect& box);
    bool press_key(const MAA_RES_NS::Action::KeyParam& param);
    bool input_text(const MAA_RES_NS::Action::TextParam& param);

    bool start_app(const MAA_RES_NS::Action::AppParam& param);
    bool stop_app(const MAA_RES_NS::Action::AppParam& param);
    bool custom_action(const MAA_RES_NS::Action::CustomParam& param, const cv::Rect& box, MaaRecoId reco_id, const std::string& name);

    void wait_freezes(const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& box);

    cv::Rect get_target_rect(const MAA_RES_NS::Action::Target target, const cv::Rect& box);

private:
    MAA_CTRL_NS::ControllerAgent* controller();

    void sleep(unsigned ms) const;
    void sleep(std::chrono::milliseconds ms) const;

private:
    Tasker* tasker_ = nullptr;
    Context& context_;
};

MAA_TASK_NS_END
