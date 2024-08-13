#include "MaaFramework/Instance/MaaTasker.h"

#include "Tasker/Tasker.h"
#include "Utils/Logger.h"

MaaTasker* MaaTaskerCreate(MaaNotificationCallback callback, void* callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    return new MAA_NS::Tasker(callback, callback_arg);
}

void MaaTaskerDestroy(MaaTasker* tasker)
{
    LogFunc << VAR_VOIDP(tasker);

    if (tasker == nullptr) {
        LogError << "handle is null";
        return;
    }

    delete tasker;
}

MaaBool MaaTaskerSetOption(MaaTasker* tasker, MaaTaskerOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(tasker) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    return tasker->set_option(key, value, val_size);
}

MaaBool MaaTaskerBindResource(MaaTasker* tasker, MaaResource* res)
{
    LogFunc << VAR_VOIDP(tasker) << VAR_VOIDP(res);

    if (!tasker || !res) {
        LogError << "handle is null";
        return false;
    }

    return tasker->bind_resource(res);
}

MaaBool MaaTaskerBindController(MaaTasker* tasker, MaaController* ctrl)
{
    LogFunc << VAR_VOIDP(tasker) << VAR_VOIDP(ctrl);

    if (!tasker || !ctrl) {
        LogError << "handle is null";
        return false;
    }

    return tasker->bind_controller(ctrl);
}

MaaBool MaaTaskerInited(MaaTasker* tasker)
{
    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    return tasker->inited();
}

MaaTaskId MaaTaskerPostPipeline(MaaTasker* tasker, const char* entry, const char* pipeline_override)
{
    LogFunc << VAR_VOIDP(tasker) << VAR(entry) << VAR(pipeline_override);

    if (!tasker) {
        LogError << "handle is null";
        return MaaInvalidId;
    }
    return tasker->post_pipeline(entry, pipeline_override);
}

MaaTaskId MaaTaskerPostRecognition(MaaTasker* tasker, const char* entry, const char* pipeline_override)
{
    LogFunc << VAR_VOIDP(tasker) << VAR(entry) << VAR(pipeline_override);

    if (!tasker) {
        LogError << "handle is null";
        return MaaInvalidId;
    }
    return tasker->post_recognition(entry, pipeline_override);
}

MaaTaskId MaaTaskerPostAction(MaaTasker* tasker, const char* entry, const char* pipeline_override)
{
    LogFunc << VAR_VOIDP(tasker) << VAR(entry) << VAR(pipeline_override);

    if (!tasker) {
        LogError << "handle is null";
        return MaaInvalidId;
    }
    return tasker->post_action(entry, pipeline_override);
}

MaaBool MaaTaskerOverridePipeline(MaaTasker* tasker, MaaTaskId id, const char* pipeline_override)
{
    LogFunc << VAR_VOIDP(tasker) << VAR(id) << VAR(pipeline_override);

    if (!tasker) {
        LogError << "handle is null";
        return false;
    }
    return tasker->override_pipeline(id, pipeline_override);
}

MaaStatus MaaTaskerStatus(MaaTasker* tasker, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(tasker) << VAR(id);

    if (!tasker) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return tasker->status(id);
}

MaaStatus MaaTaskerWait(MaaTasker* tasker, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(tasker) << VAR(id);

    if (!tasker) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return tasker->wait(id);
}

MaaBool MaaTaskerRunning(MaaTasker* tasker)
{
    if (!tasker) {
        LogError << "handle is null";
        return false;
    }
    return tasker->running();
}

MaaBool MaaTaskerPostStop(MaaTasker* tasker)
{
    LogFunc << VAR_VOIDP(tasker);

    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    tasker->post_stop();
    return true;
}

MaaResource* MaaTaskerGetResource(MaaTasker* tasker)
{
    LogFunc << VAR_VOIDP(tasker);

    if (!tasker) {
        LogError << "handle is null";
        return nullptr;
    }

    return tasker->resource();
}

MaaController* MaaTaskerGetController(MaaTasker* tasker)
{
    LogFunc << VAR_VOIDP(tasker);

    if (!tasker) {
        LogError << "handle is null";
        return nullptr;
    }
    return tasker->controller();
}
