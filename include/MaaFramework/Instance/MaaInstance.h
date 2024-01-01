#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaInstanceHandle MAA_FRAMEWORK_API MaaCreate(MaaInstanceCallback callback, MaaCallbackTransparentArg callback_arg);
    void MAA_FRAMEWORK_API MaaDestroy(MaaInstanceHandle inst);
    MaaBool MAA_FRAMEWORK_API MaaSetOption(MaaInstanceHandle inst, MaaInstOption key, MaaOptionValue value,
                                           MaaOptionValueSize val_size);

    MaaBool MAA_FRAMEWORK_API MaaBindResource(MaaInstanceHandle inst, MaaResourceHandle res);
    MaaBool MAA_FRAMEWORK_API MaaBindController(MaaInstanceHandle inst, MaaControllerHandle ctrl);
    MaaBool MAA_FRAMEWORK_API MaaInited(MaaInstanceHandle inst);

    MaaBool MAA_FRAMEWORK_API MaaRegisterCustomRecognizer(MaaInstanceHandle inst, MaaStringView name,
                                                          MaaCustomRecognizerHandle recognizer,
                                                          MaaTransparentArg recognizer_arg);
    MaaBool MAA_FRAMEWORK_API MaaUnregisterCustomRecognizer(MaaInstanceHandle inst, MaaStringView name);
    MaaBool MAA_FRAMEWORK_API MaaClearCustomRecognizer(MaaInstanceHandle inst);

    MaaBool MAA_FRAMEWORK_API MaaRegisterCustomAction(MaaInstanceHandle inst, MaaStringView name,
                                                      MaaCustomActionHandle action, MaaTransparentArg action_arg);
    MaaBool MAA_FRAMEWORK_API MaaUnregisterCustomAction(MaaInstanceHandle inst, MaaStringView name);
    MaaBool MAA_FRAMEWORK_API MaaClearCustomAction(MaaInstanceHandle inst);

    MaaTaskId MAA_FRAMEWORK_API MaaPostTask(MaaInstanceHandle inst, MaaStringView entry, MaaStringView param);
    MaaBool MAA_FRAMEWORK_API MaaSetTaskParam(MaaInstanceHandle inst, MaaTaskId id, MaaStringView param);

    MaaStatus MAA_FRAMEWORK_API MaaTaskStatus(MaaInstanceHandle inst, MaaTaskId id);
    MaaStatus MAA_FRAMEWORK_API MaaWaitTask(MaaInstanceHandle inst, MaaTaskId id);
    MaaBool MAA_FRAMEWORK_API MaaTaskAllFinished(MaaInstanceHandle inst);

    MaaBool MAA_FRAMEWORK_API MaaPostStop(MaaInstanceHandle inst);
    // Deprecated, please use MaaPostStop instead.
    MaaBool MAA_FRAMEWORK_API MaaStop(MaaInstanceHandle inst);

    MaaResourceHandle MAA_FRAMEWORK_API MaaGetResource(MaaInstanceHandle inst);
    MaaControllerHandle MAA_FRAMEWORK_API MaaGetController(MaaInstanceHandle inst);

#ifdef __cplusplus
}
#endif
