#pragma once

#include <stack>
#include <string_view>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Instance/InstanceInternalAPI.hpp"
#include "Resource/PipelineResMgr.h"
#include "Resource/PipelineTypes.h"
#include "Task/TaskDataMgr.h"

MAA_TASK_NS_BEGIN

class SyncContext : public MaaSyncContextAPI
{
public:
    explicit SyncContext(InstanceInternalAPI* inst);
    virtual ~SyncContext() override = default;

public: // from MaaSyncContextAPI
    virtual bool run_task(std::string task, std::string_view param) override;
    virtual bool run_recognizer(
        cv::Mat image,
        std::string task,
        std::string_view param,
        /*out*/ cv::Rect& box,
        /*out*/ std::string& detail) override;
    virtual bool run_action(
        std::string task,
        std::string_view param,
        cv::Rect cur_box,
        std::string cur_detail) override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;
    virtual bool press_key(int keycode) override;
    virtual bool input_text(std::string_view text) override;
    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;
    virtual cv::Mat screencap() override;

    virtual MaaInstanceHandle instance() override { return dynamic_cast<MaaInstanceHandle>(inst_); }

    virtual MaaResourceHandle resource() override
    {
        return instance() ? instance()->resource() : nullptr;
    }

    virtual MaaControllerHandle controller() override
    {
        return instance() ? instance()->controller() : nullptr;
    }

private:
    InstanceInternalAPI* inst_ = nullptr;
};

MAA_TASK_NS_END
