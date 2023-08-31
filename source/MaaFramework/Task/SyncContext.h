#pragma once
#include <string_view>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Instance/InstanceInternalAPI.hpp"
#include "Resource/PipelineConfig.h"
#include "Resource/PipelineTypes.h"

#include <stack>

MAA_TASK_NS_BEGIN

class SyncContext : public MaaSyncContextAPI, public MaaInstanceSink
{
public:
    SyncContext(InstanceInternalAPI* inst);
    virtual ~SyncContext() override = default;

public: // from MaaSyncContextAPI
    virtual bool run_task(std::string task, std::string_view param) override;

    virtual void click(int x, int y) override;
    virtual void swipe(std::vector<int> x_steps, std::vector<int> y_steps, std::vector<int> step_delay) override;
    virtual cv::Mat screencap() override;

    virtual std::string task_result(const std::string& task_name) const override;

    virtual MaaInstanceHandle instance() override { return dynamic_cast<MaaInstanceHandle>(inst_); }
    virtual MaaResourceHandle resource() override { return instance() ? instance()->resource() : nullptr; }
    virtual MaaControllerHandle controller() override { return instance() ? instance()->controller() : nullptr; }

public: // from MaaInstanceSink
    virtual void on_stop() override { need_exit_ = true; }

private:
    InstanceStatus* status() const { return inst_ ? inst_->status() : nullptr; }

    InstanceInternalAPI* inst_ = nullptr;
    bool need_exit_ = false;
};

MAA_TASK_NS_END
