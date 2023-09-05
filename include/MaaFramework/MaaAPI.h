#pragma once

#include "MaaDef.h"
#include "MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Utils */
    MaaStringView MAA_FRAMEWORK_API MaaVersion();

    MaaStringBufferHandle MAA_FRAMEWORK_API MaaCreateStringBuffer();
    void MAA_FRAMEWORK_API MaaDestroyStringBuffer(MaaStringBufferHandle handle);
    MaaStringView MAA_FRAMEWORK_API MaaGetString(MaaStringBufferHandle handle);
    MaaSize MAA_FRAMEWORK_API MaaGetStringSize(MaaStringBufferHandle handle);
    MaaBool MAA_FRAMEWORK_API MaaSetString(MaaStringBufferHandle handle, MaaStringView str);
    MaaBool MAA_FRAMEWORK_API MaaSetStringEx(MaaStringBufferHandle handle, MaaStringView str, MaaSize size);

    MaaImageBufferHandle MAA_FRAMEWORK_API MaaCreateImageBuffer();
    void MAA_FRAMEWORK_API MaaDestroyImageBuffer(MaaImageBufferHandle handle);

    typedef void* MaaImageRawData;
    MaaImageRawData MAA_FRAMEWORK_API MaaGetImageRawData(MaaImageBufferHandle handle);
    int32_t MAA_FRAMEWORK_API MaaGetImageWidth(MaaImageBufferHandle handle);
    int32_t MAA_FRAMEWORK_API MaaGetImageHeight(MaaImageBufferHandle handle);
    int32_t MAA_FRAMEWORK_API MaaGetImageType(MaaImageBufferHandle handle);
    MaaBool MAA_FRAMEWORK_API MaaSetImageRawData(MaaImageBufferHandle handle, MaaImageRawData data, int32_t width,
                                                 int32_t height, int32_t type);

    typedef uint8_t* MaaImageEncodedData;
    MaaImageEncodedData MAA_FRAMEWORK_API MaaGetImageEncoded(MaaImageBufferHandle handle);
    MaaSize MAA_FRAMEWORK_API MaaGetImageEncodedSize(MaaImageBufferHandle handle);
    MaaBool MAA_FRAMEWORK_API MaaSetImageEncoded(MaaImageBufferHandle handle, MaaImageEncodedData data, MaaSize size);

    MaaBool MAA_FRAMEWORK_API MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value,
                                                 MaaOptionValueSize val_size);

    /* Resource */

    MaaResourceHandle MAA_FRAMEWORK_API MaaResourceCreate(MaaResourceCallback callback,
                                                          MaaCallbackTransparentArg callback_arg);
    void MAA_FRAMEWORK_API MaaResourceDestroy(MaaResourceHandle res);

    MaaResId MAA_FRAMEWORK_API MaaResourcePostPath(MaaResourceHandle res, MaaStringView path);
    MaaStatus MAA_FRAMEWORK_API MaaResourceStatus(MaaResourceHandle res, MaaResId id);
    MaaStatus MAA_FRAMEWORK_API MaaResourceWait(MaaResourceHandle res, MaaResId id);
    MaaBool MAA_FRAMEWORK_API MaaResourceLoaded(MaaResourceHandle res);

    MaaBool MAA_FRAMEWORK_API MaaResourceSetOption(MaaResourceHandle res, MaaResOption key, MaaOptionValue value,
                                                   MaaOptionValueSize val_size);
    MaaBool MAA_FRAMEWORK_API MaaResourceGetHash(MaaResourceHandle res, /* out */ MaaStringBufferHandle buffer);

    /* Controller */

    MaaControllerHandle MAA_FRAMEWORK_API MaaAdbControllerCreate(MaaStringView adb_path, MaaStringView address,
                                                                 MaaAdbControllerType type, MaaStringView config,
                                                                 MaaControllerCallback callback,
                                                                 MaaCallbackTransparentArg callback_arg);
    MaaControllerHandle MAA_FRAMEWORK_API MaaCustomControllerCreate(MaaCustomControllerHandle handle,
                                                                    MaaControllerCallback callback,
                                                                    MaaCallbackTransparentArg callback_arg);
    MaaControllerHandle MAA_FRAMEWORK_API MaaThriftControllerCreate(MaaStringView param, MaaControllerCallback callback,
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

    MaaBool MAA_FRAMEWORK_API MaaControllerGetImage(MaaControllerHandle ctrl, /* out */ MaaImageBufferHandle buffer);
    MaaBool MAA_FRAMEWORK_API MaaControllerGetUUID(MaaControllerHandle ctrl, /* out */ MaaStringBufferHandle buffer);

    /* Instance */

    MaaInstanceHandle MAA_FRAMEWORK_API MaaCreate(MaaInstanceCallback callback, MaaCallbackTransparentArg callback_arg);
    void MAA_FRAMEWORK_API MaaDestroy(MaaInstanceHandle inst);
    MaaBool MAA_FRAMEWORK_API MaaSetOption(MaaInstanceHandle inst, MaaInstOption key, MaaOptionValue value,
                                           MaaOptionValueSize val_size);

    MaaBool MAA_FRAMEWORK_API MaaBindResource(MaaInstanceHandle inst, MaaResourceHandle res);
    MaaBool MAA_FRAMEWORK_API MaaBindController(MaaInstanceHandle inst, MaaControllerHandle ctrl);
    MaaBool MAA_FRAMEWORK_API MaaInited(MaaInstanceHandle inst);

    MaaBool MAA_FRAMEWORK_API MaaRegisterCustomRecognizer(MaaInstanceHandle inst, MaaStringView name,
                                                          MaaCustomRecognizerHandle recognizer);
    MaaBool MAA_FRAMEWORK_API MaaUnregisterCustomRecognizer(MaaInstanceHandle inst, MaaStringView name);
    MaaBool MAA_FRAMEWORK_API MaaClearCustomRecognizer(MaaInstanceHandle inst);

    MaaBool MAA_FRAMEWORK_API MaaRegisterCustomAction(MaaInstanceHandle inst, MaaStringView name,
                                                      MaaCustomActionHandle action);
    MaaBool MAA_FRAMEWORK_API MaaUnregisterCustomAction(MaaInstanceHandle inst, MaaStringView name);
    MaaBool MAA_FRAMEWORK_API MaaClearCustomAction(MaaInstanceHandle inst);

    MaaTaskId MAA_FRAMEWORK_API MaaPostTask(MaaInstanceHandle inst, MaaStringView entry, MaaStringView param);
    MaaBool MAA_FRAMEWORK_API MaaSetTaskParam(MaaInstanceHandle inst, MaaTaskId id, MaaStringView param);

    MaaStatus MAA_FRAMEWORK_API MaaTaskStatus(MaaInstanceHandle inst, MaaTaskId id);
    MaaStatus MAA_FRAMEWORK_API MaaWaitTask(MaaInstanceHandle inst, MaaTaskId id);
    MaaBool MAA_FRAMEWORK_API MaaTaskAllFinished(MaaInstanceHandle inst);

    void MAA_FRAMEWORK_API MaaStop(MaaInstanceHandle inst);

    MaaResourceHandle MAA_FRAMEWORK_API MaaGetResource(MaaInstanceHandle inst);
    MaaControllerHandle MAA_FRAMEWORK_API MaaGetController(MaaInstanceHandle inst);

    /* SyncContext */

    MaaBool MAA_FRAMEWORK_API MaaSyncContextRunTask(MaaSyncContextHandle sync_context, MaaStringView task,
                                                    MaaStringView param);
    MaaBool MAA_FRAMEWORK_API MaaSyncContextRunRecognizer(MaaSyncContextHandle sync_context, MaaImageBufferHandle image,
                                                          MaaStringView task, MaaStringView task_param,
                                                          /* out */ MaaRectHandle out_box,
                                                          /* out */ MaaStringBufferHandle detail_buff);
    MaaBool MAA_FRAMEWORK_API MaaSyncContextRunAction(MaaSyncContextHandle sync_context, MaaStringView task,
                                                      MaaStringView task_param, MaaRectHandle cur_box,
                                                      MaaStringView cur_rec_detail);
    void MAA_FRAMEWORK_API MaaSyncContextClick(MaaSyncContextHandle sync_context, int32_t x, int32_t y);
    void MAA_FRAMEWORK_API MaaSyncContextSwipe(MaaSyncContextHandle sync_context, int32_t* x_steps_buff,
                                               int32_t* y_steps_buff, int32_t* step_delay_buff, MaaSize buff_size);
    MaaBool MAA_FRAMEWORK_API MaaSyncContextScreencap(MaaSyncContextHandle sync_context,
                                                      /* out */ MaaImageBufferHandle buffer);
    MaaBool MAA_FRAMEWORK_API MaaSyncContextGetTaskResult(MaaSyncContextHandle sync_context, MaaStringView task,
                                                          /* out */ MaaStringBufferHandle buffer);

#ifdef __cplusplus
}
#endif
