#pragma once

#include <stack>
#include <string_view>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Recognizer.h"
#include "Resource/PipelineResMgr.h"
#include "Resource/PipelineTypes.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

class Actuator
{
public:
    using PipelineData = MAA_RES_NS::PipelineData;
    using PreTaskBoxes = std::map<std::string, cv::Rect>;

public:
    Actuator(Tasker* tasker);

    bool run(const Recognizer::Hit& reco_hit, const json::value& reco_detail, const PipelineData& pipeline_data);

private:
    bool click(const MAA_RES_NS::Action::ClickParam& param, const cv::Rect& cur_box);
    bool swipe(const MAA_RES_NS::Action::SwipeParam& param, const cv::Rect& cur_box);
    bool press_key(const MAA_RES_NS::Action::KeyParam& param);
    bool input_text(const MAA_RES_NS::Action::TextParam& param);

    bool start_app(const MAA_RES_NS::Action::AppParam& param);
    bool stop_app(const MAA_RES_NS::Action::AppParam& param);
    bool custom_action(
        const std::string& task_name,
        const MAA_RES_NS::Action::CustomParam& param,
        const cv::Rect& cur_box,
        const json::value& cur_rec_detail);

    void wait_freezes(const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& cur_box);

    cv::Rect get_target_rect(const MAA_RES_NS::Action::Target target, const cv::Rect& cur_box);

private:
    MAA_CTRL_NS::ControllerAgent* controller() { return inst_ ? inst_->inter_controller() : nullptr; }

    void sleep(unsigned ms) const;
    void sleep(std::chrono::milliseconds ms) const;

private:
    Tasker* tasker_ = nullptr;
};

MAA_TASK_NS_END
