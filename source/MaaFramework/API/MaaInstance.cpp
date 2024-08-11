#include "MaaFramework/Instance/MaaScheduler.h"

#include "Instance/InstanceMgr.h"
#include "Utils/Logger.h"

MaaScheduler* MaaCreate(MaaInstanceCallback callback, MaaTransparentArg callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    return new MAA_NS::InstanceMgr(callback, callback_arg);
}

void MaaDestroy(MaaScheduler* inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (inst == nullptr) {
        LogError << "handle is null";
        return;
    }

    delete inst;
}

MaaBool MaaSetOption(MaaScheduler* inst, MaaSchedOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(inst) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->set_option(key, value, val_size);
}

MaaBool MaaBindResource(MaaScheduler* inst, MaaResource* res)
{
    LogFunc << VAR_VOIDP(inst) << VAR_VOIDP(res);

    if (!inst || !res) {
        LogError << "handle is null";
        return false;
    }

    return inst->bind_resource(res);
}

MaaBool MaaBindController(MaaScheduler* inst, MaaController* ctrl)
{
    LogFunc << VAR_VOIDP(inst) << VAR_VOIDP(ctrl);

    if (!inst || !ctrl) {
        LogError << "handle is null";
        return false;
    }

    return inst->bind_controller(ctrl);
}

MaaBool MaaInited(MaaScheduler* inst)
{
    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->inited();
}

MaaBool MaaRegisterCustomRecognizer(
    MaaScheduler* inst,
    const char* name,
    MaaCustomRecognizerHandle recognizer,
    MaaTransparentArg recognizer_arg)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name) << VAR_VOIDP(recognizer) << VAR_VOIDP(recognizer_arg);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->register_custom_recognizer(name, recognizer, recognizer_arg);
}

MaaBool MaaUnregisterCustomRecognizer(MaaScheduler* inst, const char* name)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->unregister_custom_recognizer(name);
}

MaaBool MaaClearCustomRecognizer(MaaScheduler* inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    inst->clear_custom_recognizer();
    return true;
}

MaaBool MaaRegisterCustomAction(MaaScheduler* inst, const char* name, MaaCustomActionHandle action, MaaTransparentArg action_arg)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name) << VAR_VOIDP(action) << VAR_VOIDP(action_arg);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->register_custom_action(name, action, action_arg);
}

MaaBool MaaUnregisterCustomAction(MaaScheduler* inst, const char* name)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->unregister_custom_action(name);
}

MaaBool MaaClearCustomAction(MaaScheduler* inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    inst->clear_custom_action();
    return true;
}

MaaTaskId MaaPostTask(MaaScheduler* inst, const char* entry, const char* param)
{
    LogFunc << VAR_VOIDP(inst) << VAR(entry) << VAR(param);

    if (!inst) {
        LogError << "handle is null";
        return MaaInvalidId;
    }
    return inst->post_pipeline(entry, param);
}

MaaTaskId MaaPostRecognition(MaaScheduler* inst, const char* entry, const char* param)
{
    LogFunc << VAR_VOIDP(inst) << VAR(entry) << VAR(param);

    if (!inst) {
        LogError << "handle is null";
        return MaaInvalidId;
    }
    return inst->post_recognition(entry, param);
}

MaaTaskId MaaPostAction(MaaScheduler* inst, const char* entry, const char* param)
{
    LogFunc << VAR_VOIDP(inst) << VAR(entry) << VAR(param);

    if (!inst) {
        LogError << "handle is null";
        return MaaInvalidId;
    }
    return inst->post_action(entry, param);
}

MaaBool MaaSetTaskParam(MaaScheduler* inst, MaaTaskId id, const char* param)
{
    LogFunc << VAR_VOIDP(inst) << VAR(id) << VAR(param);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }
    return inst->set_task_param(id, param);
}

MaaStatus MaaTaskStatus(MaaScheduler* inst, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(inst) << VAR(id);

    if (!inst) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return inst->task_status(id);
}

MaaStatus MaaWaitTask(MaaScheduler* inst, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(inst) << VAR(id);

    if (!inst) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return inst->task_wait(id);
}

MaaBool MaaTaskAllFinished(MaaScheduler* inst)
{
    LogWarn << __FUNCTION__ << "is deprecated, use !MaaRunning instead.";

    return !MaaRunning(inst);
}

MaaBool MaaRunning(MaaScheduler* inst)
{
    if (!inst) {
        LogError << "handle is null";
        return false;
    }
    return inst->running();
}

MaaBool MaaPostStop(MaaScheduler* inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    inst->post_stop();
    return true;
}

MaaBool MaaStop(MaaScheduler* inst)
{
    LogWarn << __FUNCTION__ << "is deprecated, use MaaPostStop instead.";
    return MaaPostStop(inst);
}

MaaResource* MaaGetResource(MaaScheduler* inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return nullptr;
    }

    return inst->resource();
}

MaaController* MaaGetController(MaaScheduler* inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return nullptr;
    }
    return inst->controller();
}
