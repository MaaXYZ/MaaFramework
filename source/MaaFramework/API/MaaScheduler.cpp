#include "MaaFramework/Instance/MaaScheduler.h"

#include "Instance/Scheduler.h"
#include "Utils/Logger.h"

MaaScheduler* MaaSchedulerCreate(MaaNotificationCallback callback, MaaTransparentArg callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    return new MAA_NS::Scheduler(callback, callback_arg);
}

void MaaSchedulerDestroy(MaaScheduler* sched)
{
    LogFunc << VAR_VOIDP(sched);

    if (sched == nullptr) {
        LogError << "handle is null";
        return;
    }

    delete sched;
}

MaaBool MaaSchedulerSetOption(MaaScheduler* sched, MaaSchedOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(sched) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!sched) {
        LogError << "handle is null";
        return false;
    }

    return sched->set_option(key, value, val_size);
}

MaaBool MaaSchedulerBindResource(MaaScheduler* sched, MaaResource* res)
{
    LogFunc << VAR_VOIDP(sched) << VAR_VOIDP(res);

    if (!sched || !res) {
        LogError << "handle is null";
        return false;
    }

    return sched->bind_resource(res);
}

MaaBool MaaSchedulerBindController(MaaScheduler* sched, MaaController* ctrl)
{
    LogFunc << VAR_VOIDP(sched) << VAR_VOIDP(ctrl);

    if (!sched || !ctrl) {
        LogError << "handle is null";
        return false;
    }

    return sched->bind_controller(ctrl);
}

MaaBool MaaSchedulerInited(MaaScheduler* sched)
{
    if (!sched) {
        LogError << "handle is null";
        return false;
    }

    return sched->inited();
}

MaaTaskId MaaSchedulerPostTask(MaaScheduler* sched, const char* entry, const char* param)
{
    LogFunc << VAR_VOIDP(sched) << VAR(entry) << VAR(param);

    if (!sched) {
        LogError << "handle is null";
        return MaaInvalidId;
    }
    return sched->post_pipeline(entry, param);
}

MaaTaskId MaaSchedulerPostRecognition(MaaScheduler* sched, const char* entry, const char* param)
{
    LogFunc << VAR_VOIDP(sched) << VAR(entry) << VAR(param);

    if (!sched) {
        LogError << "handle is null";
        return MaaInvalidId;
    }
    return sched->post_recognition(entry, param);
}

MaaTaskId MaaSchedulerPostAction(MaaScheduler* sched, const char* entry, const char* param)
{
    LogFunc << VAR_VOIDP(sched) << VAR(entry) << VAR(param);

    if (!sched) {
        LogError << "handle is null";
        return MaaInvalidId;
    }
    return sched->post_action(entry, param);
}

MaaBool MaaSchedulerSetParam(MaaScheduler* sched, MaaTaskId id, const char* param)
{
    LogFunc << VAR_VOIDP(sched) << VAR(id) << VAR(param);

    if (!sched) {
        LogError << "handle is null";
        return false;
    }
    return sched->set_param(id, param);
}

MaaStatus MaaSchedulerStatus(MaaScheduler* sched, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(sched) << VAR(id);

    if (!sched) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return sched->status(id);
}

MaaStatus MaaSchedulerWait(MaaScheduler* sched, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(sched) << VAR(id);

    if (!sched) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return sched->wait(id);
}

MaaBool MaaSchedulerRunning(MaaScheduler* sched)
{
    if (!sched) {
        LogError << "handle is null";
        return false;
    }
    return sched->running();
}

MaaBool MaaSchedulerPostStop(MaaScheduler* sched)
{
    LogFunc << VAR_VOIDP(sched);

    if (!sched) {
        LogError << "handle is null";
        return false;
    }

    sched->post_stop();
    return true;
}

MaaResource* MaaSchedulerGetResource(MaaScheduler* sched)
{
    LogFunc << VAR_VOIDP(sched);

    if (!sched) {
        LogError << "handle is null";
        return nullptr;
    }

    return sched->resource();
}

MaaController* MaaSchedulerGetController(MaaScheduler* sched)
{
    LogFunc << VAR_VOIDP(sched);

    if (!sched) {
        LogError << "handle is null";
        return nullptr;
    }
    return sched->controller();
}
