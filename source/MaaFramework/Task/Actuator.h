#pragma once

#include <stack>
#include <string_view>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Instance/InstanceInternalAPI.hpp"
#include "Recognizer.h"
#include "Resource/PipelineResMgr.h"
#include "Resource/PipelineTypes.h"

MAA_TASK_NS_BEGIN

class Actuator
{
public:
    using TaskData = MAA_RES_NS::TaskData;
    using PreTaskBoxes = std::map<std::string, cv::Rect>;

public:
    Actuator(InstanceInternalAPI* inst, const PreTaskBoxes& boxes = {});

    bool
        run(const Recognizer::Hit& reco_hit,
            const json::value& reco_detail,
            const TaskData& task_data);

    MaaRunningId uid() const { return uid_; }

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
    MAA_CTRL_NS::ControllerAgent* controller()
    {
        return inst_ ? inst_->inter_controller() : nullptr;
    }

    void sleep(unsigned ms) const;
    void sleep(std::chrono::milliseconds ms) const;

private:
    InstanceInternalAPI* inst_ = nullptr;
    const PreTaskBoxes& pre_task_boxes_;

    const MaaRunningId uid_ = ++s_global_uid;

    inline static std::atomic<MaaRunningId> s_global_uid = 0;
};

MAA_TASK_NS_END
