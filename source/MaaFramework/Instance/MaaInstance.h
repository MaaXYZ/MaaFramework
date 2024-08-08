#pragma once

#include <mutex>

#include "Base/AsyncRunner.hpp"
#include "InstanceCache.h"
#include "Task/PipelineTask.h"
#include "Utils/MessageNotifier.hpp"

MAA_NS_BEGIN

class MaaInstance
{
public:
    MaaInstance(MaaNotificationCallback callback, MaaCallbackTransparentArg callback_arg);
    ~MaaInstance();

    bool bind_resource(MaaResourceAPI* resource);
    bool bind_controller(MaaControllerAPI* controller);
    bool inited() const;

    bool set_option(MaaInstOption key, MaaOptionValue value, MaaOptionValueSize val_size);

    MaaTaskId post_pipeline(std::string entry, std::string_view param);
    MaaTaskId post_recognition(std::string entry, std::string_view param);
    MaaTaskId post_action(std::string entry, std::string_view param);
    bool set_task_param(MaaTaskId task_id, std::string_view param);

    bool register_custom_recognizer(std::string name, MaaCustomRecognizerHandle handle, MaaTransparentArg handle_arg);
    bool unregister_custom_recognizer(std::string name);
    void clear_custom_recognizer();
    bool register_custom_action(std::string name, MaaCustomActionHandle handle, MaaTransparentArg handle_arg);
    bool unregister_custom_action(std::string name);
    void clear_custom_action();

    MaaStatus task_status(MaaTaskId task_id) const;
    MaaStatus task_wait(MaaTaskId task_id) const;

    MaaBool running() const;
    void post_stop();

    MaaResourceHandle resource();
    MaaControllerHandle controller();

    MAA_RES_NS::ResourceMgr* inter_resource();
    MAA_CTRL_NS::ControllerAgent* inter_controller();
    void notify(std::string_view msg, const json::value& details = json::value());
    InstanceCache* cache();

private:
    using TaskPtr = std::shared_ptr<TaskNS::PipelineTask>;
    using TaskId = AsyncRunner<TaskPtr>::Id;

    TaskPtr make_task(std::string entry, std::string_view param);
    bool run_task(TaskId id, TaskPtr task_ptr);
    bool check_stop();

private:
    MaaResourceAPI* resource_ = nullptr;
    MaaControllerAPI* controller_ = nullptr;
    bool need_to_stop_ = false;

    InstanceCache cache_;

    std::unique_ptr<AsyncRunner<TaskPtr>> task_runner_ = nullptr;
    MessageNotifier<MaaNotificationCallback> notifier;
};

MAA_NS_END
