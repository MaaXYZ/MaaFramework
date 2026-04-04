#pragma once

#include <atomic>
#include <string_view>

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "Controller/ControllerAgent.h"
#include "Resource/PipelineTypes.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

class Context;

class ActionHelper : public NonCopyable
{
public:
    struct WaitFreezesNotifyContext
    {
        std::string name;
        std::string phase;
        json::value focus;
    };

public:
    static MaaWfId generate_wf_id() { return ++s_global_wf_id; }

    explicit ActionHelper(Context* context);

    bool wait_freezes(const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& ref_box, const WaitFreezesNotifyContext& noti_ctx = { });

    cv::Rect get_target_rect(const MAA_RES_NS::Action::Target& target, const cv::Rect& box = { });

private:
    cv::Rect get_rect_from_node(const std::string& node_name) const;
    void notify(std::string_view msg, const json::value& detail);
    Tasker* tasker() const;
    MAA_CTRL_NS::ControllerAgent* controller();

private:
    inline static std::atomic<MaaWfId> s_global_wf_id = kWfIdBase;

    Context* context_ = nullptr;
};

MAA_TASK_NS_END
