#pragma once

#include <atomic>
#include <random>

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "Common/MaaTypes.h"
#include "Common/TaskResultTypes.h"
#include "Controller/ControllerAgent.h"
#include "Resource/PipelineTypes.h"
#include "Task/Context.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

class Actuator : public NonCopyable
{
public:
    using PreTaskBoxes = std::map<std::string, cv::Rect>;

public:
    Actuator(Tasker* tasker, Context& context);

    ActionResult run(const cv::Rect& reco_hit, MaaRecoId reco_id, const PipelineData& pipeline_data, const std::string& entry);

    MaaActId get_id() const { return action_id_; }

private:
    static cv::Point rand_point(const cv::Rect& r);
    static std::mt19937 rand_engine_;
    inline static std::atomic<MaaActId> s_global_action_id = kActIdBase;

private:
    ActionResult execute_action(const cv::Rect& reco_hit, MaaRecoId reco_id, const PipelineData& pipeline_data, const std::string& entry);

    ActionResult click(const MAA_RES_NS::Action::ClickParam& param, const cv::Rect& box, const std::string& name);
    ActionResult long_press(const MAA_RES_NS::Action::LongPressParam& param, const cv::Rect& box, const std::string& name);
    ActionResult swipe(const MAA_RES_NS::Action::SwipeParam& param, const cv::Rect& box, const std::string& name);
    ActionResult multi_swipe(const MAA_RES_NS::Action::MultiSwipeParam& param, const cv::Rect& box, const std::string& name);
    ActionResult touch_down(const MAA_RES_NS::Action::TouchParam& param, const cv::Rect& box, const std::string& name);
    ActionResult touch_move(const MAA_RES_NS::Action::TouchParam& param, const cv::Rect& box, const std::string& name);
    ActionResult touch_up(const MAA_RES_NS::Action::TouchUpParam& param, const std::string& name);
    ActionResult click_key(const MAA_RES_NS::Action::ClickKeyParam& param, const std::string& name);
    ActionResult long_press_key(const MAA_RES_NS::Action::LongPressKeyParam& param, const std::string& name);
    ActionResult key_down(const MAA_RES_NS::Action::KeyParam& param, const std::string& name);
    ActionResult key_up(const MAA_RES_NS::Action::KeyParam& param, const std::string& name);
    ActionResult input_text(const MAA_RES_NS::Action::InputTextParam& param, const std::string& name);
    ActionResult scroll(const MAA_RES_NS::Action::ScrollParam& param, const std::string& name);
    ActionResult shell(const MAA_RES_NS::Action::ShellParam& param, const std::string& name);

    ActionResult start_app(const MAA_RES_NS::Action::AppParam& param, const std::string& name);
    ActionResult stop_app(const MAA_RES_NS::Action::AppParam& param, const std::string& name);

    ActionResult
        command(const MAA_RES_NS::Action::CommandParam& param, const cv::Rect& box, const std::string& name, const std::string& entry);
    ActionResult
        custom_action(const MAA_RES_NS::Action::CustomParam& param, const cv::Rect& box, MaaRecoId reco_id, const std::string& name);
    ActionResult do_nothing(const std::string& name);
    ActionResult stop_task(const std::string& name);

    void wait_freezes(const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& box);

    cv::Rect get_target_rect(const MAA_RES_NS::Action::Target target, const cv::Rect& box);

private:
    MAA_CTRL_NS::ControllerAgent* controller();

    void sleep(unsigned ms) const;
    void sleep(std::chrono::milliseconds ms) const;

private:
    Tasker* tasker_ = nullptr;
    Context& context_;
    const MaaActId action_id_ = ++s_global_action_id;
};

MAA_TASK_NS_END
