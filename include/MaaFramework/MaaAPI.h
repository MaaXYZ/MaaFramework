#pragma once

#include "MaaFramework/MaaDef.h"
#include "MaaFramework/MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaString MAA_FRAMEWORK_API MaaVersion();
    MaaBool MAA_FRAMEWORK_API MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value,
                                                 MaaOptionValueSize val_size);

    /* Resource */

    MaaResourceHandle MAA_FRAMEWORK_API MaaResourceCreate(MaaResourceCallback callback,
                                                          MaaCallbackTransparentArg callback_arg);
    void MAA_FRAMEWORK_API MaaResourceDestroy(MaaResourceHandle res);

    MaaResId MAA_FRAMEWORK_API MaaResourcePostResource(MaaResourceHandle res, MaaString path);
    MaaStatus MAA_FRAMEWORK_API MaaResourceStatus(MaaResourceHandle res, MaaResId id);
    MaaStatus MAA_FRAMEWORK_API MaaResourceWait(MaaResourceHandle res, MaaResId id);
    MaaBool MAA_FRAMEWORK_API MaaResourceLoaded(MaaResourceHandle res);

    MaaBool MAA_FRAMEWORK_API MaaResourceSetOption(MaaResourceHandle res, MaaResOption key, MaaOptionValue value,
                                                   MaaOptionValueSize val_size);
    MaaSize MAA_FRAMEWORK_API MaaResourceGetHash(MaaResourceHandle res, char* buff, MaaSize buff_size);

    /* Controller */

    MaaControllerHandle MAA_FRAMEWORK_API MaaAdbControllerCreate(MaaString adb_path, MaaString address,
                                                                 MaaAdbControllerType type, MaaJsonString config,
                                                                 MaaControllerCallback callback,
                                                                 MaaCallbackTransparentArg callback_arg);
    MaaControllerHandle MAA_FRAMEWORK_API MaaCustomControllerCreate(MaaCustomControllerHandle handle,
                                                                    MaaControllerCallback callback,
                                                                    MaaCallbackTransparentArg callback_arg);
    MaaControllerHandle MAA_FRAMEWORK_API MaaThriftControllerCreate(MaaString param, MaaControllerCallback callback,
                                                                    MaaCallbackTransparentArg callback_arg);

    void MAA_FRAMEWORK_API MaaControllerDestroy(MaaControllerHandle ctrl);

    MaaBool MAA_FRAMEWORK_API MaaControllerSetOption(MaaControllerHandle ctrl, MaaCtrlOption key, MaaOptionValue value,
                                                     MaaOptionValueSize val_size);

    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostConnection(MaaControllerHandle ctrl);
    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostClick(MaaControllerHandle ctrl, int32_t x, int32_t y);
    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostSwipe(MaaControllerHandle ctrl, int32_t* x_steps_buff,
                                                       int32_t* y_steps_buff, int32_t* step_delay_buff,
                                                       MaaSize buff_size);
    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostScreencap(MaaControllerHandle ctrl);

    MaaStatus MAA_FRAMEWORK_API MaaControllerStatus(MaaControllerHandle ctrl, MaaCtrlId id);
    MaaStatus MAA_FRAMEWORK_API MaaControllerWait(MaaControllerHandle ctrl, MaaCtrlId id);
    MaaBool MAA_FRAMEWORK_API MaaControllerConnected(MaaControllerHandle ctrl);

    MaaSize MAA_FRAMEWORK_API MaaControllerGetImage(MaaControllerHandle ctrl, void* buff, MaaSize buff_size);
    MaaSize MAA_FRAMEWORK_API MaaControllerGetUUID(MaaControllerHandle ctrl, char* buff, MaaSize buff_size);

    /* Instance */

    MaaInstanceHandle MAA_FRAMEWORK_API MaaCreate(MaaInstanceCallback callback, MaaCallbackTransparentArg callback_arg);
    void MAA_FRAMEWORK_API MaaDestroy(MaaInstanceHandle inst);
    MaaBool MAA_FRAMEWORK_API MaaSetOption(MaaInstanceHandle inst, MaaInstOption key, MaaOptionValue value,
                                           MaaOptionValueSize val_size);

    MaaBool MAA_FRAMEWORK_API MaaBindResource(MaaInstanceHandle inst, MaaResourceHandle res);
    MaaBool MAA_FRAMEWORK_API MaaBindController(MaaInstanceHandle inst, MaaControllerHandle ctrl);
    MaaBool MAA_FRAMEWORK_API MaaInited(MaaInstanceHandle inst);

    MaaBool MAA_FRAMEWORK_API MaaRegisterCustomRecognizer(MaaInstanceHandle inst, MaaString name,
                                                          MaaCustomRecognizerHandle recognizer);
    MaaBool MAA_FRAMEWORK_API MaaUnregisterCustomRecognizer(MaaInstanceHandle inst, MaaString name);
    MaaBool MAA_FRAMEWORK_API MaaClearCustomRecognizer(MaaInstanceHandle inst);

    MaaBool MAA_FRAMEWORK_API MaaRegisterCustomAction(MaaInstanceHandle inst, MaaString name,
                                                      MaaCustomActionHandle action);
    MaaBool MAA_FRAMEWORK_API MaaUnregisterCustomAction(MaaInstanceHandle inst, MaaString name);
    MaaBool MAA_FRAMEWORK_API MaaClearCustomAction(MaaInstanceHandle inst);

    MaaTaskId MAA_FRAMEWORK_API MaaPostTask(MaaInstanceHandle inst, MaaString task, MaaJsonString param);
    MaaBool MAA_FRAMEWORK_API MaaSetTaskParam(MaaInstanceHandle inst, MaaTaskId id, MaaJsonString param);

    MaaStatus MAA_FRAMEWORK_API MaaTaskStatus(MaaInstanceHandle inst, MaaTaskId id);
    MaaStatus MAA_FRAMEWORK_API MaaWaitTask(MaaInstanceHandle inst, MaaTaskId id);
    MaaBool MAA_FRAMEWORK_API MaaTaskAllFinished(MaaInstanceHandle inst);

    void MAA_FRAMEWORK_API MaaStop(MaaInstanceHandle inst);

    MaaResourceHandle MAA_FRAMEWORK_API MaaGetResource(MaaInstanceHandle inst);
    MaaControllerHandle MAA_FRAMEWORK_API MaaGetController(MaaInstanceHandle inst);

    /* SyncContext */

    MaaBool MAA_FRAMEWORK_API MaaSyncContextRunTask(MaaSyncContextHandle sync_context, MaaString task,
                                                    MaaJsonString param);
    void MAA_FRAMEWORK_API MaaSyncContextClick(MaaSyncContextHandle sync_context, int32_t x, int32_t y);
    void MAA_FRAMEWORK_API MaaSyncContextSwipe(MaaSyncContextHandle sync_context, int32_t* x_steps_buff,
                                               int32_t* y_steps_buff, int32_t* step_delay_buff, MaaSize buff_size);
    MaaSize MAA_FRAMEWORK_API MaaSyncContextScreencap(MaaSyncContextHandle sync_context, void* buff, MaaSize buff_size);
    MaaSize MAA_FRAMEWORK_API MaaSyncContextGetTaskResult(MaaSyncContextHandle sync_context, MaaString task, char* buff,
                                                          MaaSize buff_size);

#ifdef __cplusplus
}
#endif
