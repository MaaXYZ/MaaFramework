#include "MaaFramework/Instance/MaaInstance.h"

#include "Instance/InstanceMgr.h"
#include "Utils/Logger.h"

MaaInstanceHandle MaaCreate(MaaInstanceCallback callback, MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    return new MAA_NS::InstanceMgr(callback, callback_arg);
}

void MaaDestroy(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (inst == nullptr) {
        LogError << "handle is null";
        return;
    }

    delete inst;
}

MaaBool MaaSetOption(MaaInstanceHandle inst, MaaInstOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(inst) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->set_option(key, value, val_size);
}

MaaBool MaaBindResource(MaaInstanceHandle inst, MaaResourceHandle res)
{
    LogFunc << VAR_VOIDP(inst) << VAR_VOIDP(res);

    if (!inst || !res) {
        LogError << "handle is null";
        return false;
    }

    return inst->bind_resource(res);
}

MaaBool MaaBindController(MaaInstanceHandle inst, MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(inst) << VAR_VOIDP(ctrl);

    if (!inst || !ctrl) {
        LogError << "handle is null";
        return false;
    }

    return inst->bind_controller(ctrl);
}

MaaBool MaaInited(MaaInstanceHandle inst)
{
    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->inited();
}

MaaBool MaaRegisterCustomRecognizer(MaaInstanceHandle inst, MaaStringView name, MaaCustomRecognizerHandle recognizer,
                                    MaaTransparentArg recognizer_arg)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name) << VAR_VOIDP(recognizer) << VAR_VOIDP(recognizer_arg);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->register_custom_recognizer(name, recognizer, recognizer_arg);
}

MaaBool MaaUnregisterCustomRecognizer(MaaInstanceHandle inst, MaaStringView name)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->unregister_custom_recognizer(name);
}

MaaBool MaaClearCustomRecognizer(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    inst->clear_custom_recognizer();
    return true;
}

MaaBool MaaRegisterCustomAction(MaaInstanceHandle inst, MaaStringView name, MaaCustomActionHandle action,
                                MaaTransparentArg action_arg)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name) << VAR_VOIDP(action) << VAR_VOIDP(action_arg);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->register_custom_action(name, action, action_arg);
}

MaaBool MaaUnregisterCustomAction(MaaInstanceHandle inst, MaaStringView name)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->unregister_custom_action(name);
}

MaaBool MaaClearCustomAction(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    inst->clear_custom_action();
    return true;
}

MaaTaskId MaaPostTask(MaaInstanceHandle inst, MaaStringView entry, MaaStringView param)
{
    LogFunc << VAR_VOIDP(inst) << VAR(entry) << VAR(param);

    if (!inst) {
        LogError << "handle is null";
        return MaaInvalidId;
    }
    return inst->post_task(entry, param);
}

MaaBool MaaSetTaskParam(MaaInstanceHandle inst, MaaTaskId id, MaaStringView param)
{
    LogFunc << VAR_VOIDP(inst) << VAR(id) << VAR(param);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }
    return inst->set_task_param(id, param);
}

MaaStatus MaaTaskStatus(MaaInstanceHandle inst, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(inst) << VAR(id);

    if (!inst) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return inst->task_status(id);
}

MaaStatus MaaWaitTask(MaaInstanceHandle inst, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(inst) << VAR(id);

    if (!inst) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return inst->task_wait(id);
}

MaaBool MaaTaskAllFinished(MaaInstanceHandle inst)
{
    // LogFunc << VAR_VOIDP(inst) << VAR(id);
    if (!inst) {
        LogError << "handle is null";
        return false;
    }
    return inst->task_all_finished();
}

MaaBool MaaStop(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    inst->stop();
    return true;
}

MaaResourceHandle MaaGetResource(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return nullptr;
    }

    return inst->resource();
}

MaaControllerHandle MaaGetController(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return nullptr;
    }
    return inst->controller();
}
