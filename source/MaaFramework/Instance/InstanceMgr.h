#pragma once

#include <mutex>

#include "API/MaaTypes.h"
#include "Base/AsyncRunner.hpp"
#include "InstanceInternalAPI.hpp"
#include "Task/PipelineTask.h"
#include "Utils/MessageNotifier.hpp"

MAA_NS_BEGIN

class InstanceMgr
    : public MaaInstanceAPI
    , public InstanceInternalAPI
{
public:
    InstanceMgr(MaaInstanceCallback callback, MaaCallbackTransparentArg callback_arg);
    virtual ~InstanceMgr() override;

public: // from MaaInstanceAPI
    virtual bool bind_resource(MaaResourceAPI* resource) override;
    virtual bool bind_controller(MaaControllerAPI* controller) override;
    virtual bool inited() const override;

    virtual bool
        set_option(MaaInstOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaTaskId post_task(std::string entry, std::string_view param) override;
    virtual bool set_task_param(MaaTaskId task_id, std::string_view param) override;

    virtual bool register_custom_recognizer(
        std::string name,
        MaaCustomRecognizerHandle handle,
        MaaTransparentArg handle_arg) override;
    virtual bool unregister_custom_recognizer(std::string name) override;
    virtual void clear_custom_recognizer() override;
    virtual bool register_custom_action(
        std::string name,
        MaaCustomActionHandle handle,
        MaaTransparentArg handle_arg) override;
    virtual bool unregister_custom_action(std::string name) override;
    virtual void clear_custom_action() override;

    virtual MaaStatus task_status(MaaTaskId task_id) const override;
    virtual MaaStatus task_wait(MaaTaskId task_id) const override;

    virtual MaaBool running() const override;
    virtual void post_stop() override;

    virtual MaaResourceHandle resource() override;
    virtual MaaControllerHandle controller() override;

public: // from InstanceInternalAPI
    virtual MAA_RES_NS::ResourceMgr* inter_resource() override;
    virtual MAA_CTRL_NS::ControllerAgent* inter_controller() override;
    virtual void notify(std::string_view msg, const json::value& details = json::value()) override;
    virtual CustomRecognizerSession* custom_recognizer_session(const std::string& name) override;
    virtual CustomActionSession* custom_action_session(const std::string& name) override;

private:
    using TaskPtr = std::shared_ptr<TaskNS::PipelineTask>;
    using TaskId = AsyncRunner<TaskPtr>::Id;

    bool run_task(TaskId id, TaskPtr task_ptr);
    bool check_stop();

private:
    MaaResourceAPI* resource_ = nullptr;
    MaaControllerAPI* controller_ = nullptr;
    bool need_to_stop_ = false;

    std::unordered_map<std::string, CustomRecognizerSession> custom_recognizer_sessions_;
    std::unordered_map<std::string, CustomActionSession> custom_action_sessions_;

    std::unique_ptr<AsyncRunner<TaskPtr>> task_runner_ = nullptr;
    MessageNotifier<MaaInstanceCallback> notifier;
};

MAA_NS_END
