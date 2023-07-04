#pragma once

#include "MaaDef.h"
#include "MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif
    MaaBool MAA_API MaaSetGlobalOption(MaaGlobalOption key, MaaString value);

    /* Resource */

    MaaResourceHandle MAA_API MaaResourceCreate(MaaString user_path, MaaResourceCallback callback, MaaCallbackTransparentArg callback_arg);
    void MAA_API MaaResourceDestroy(MaaResourceHandle* res);

    MaaResId MAA_API MaaResourcePostResource(MaaResourceHandle res, MaaString path);
    MaaStatus MAA_API MaaResourceStatus(MaaResourceHandle res, MaaResId id);
    MaaStatus MAA_API MaaResourceWait(MaaResourceHandle res, MaaResId id);
    MaaBool MAA_API MaaResourceLoaded(MaaResourceHandle res);

    MaaBool MAA_API MaaResourceSetOption(MaaResourceHandle res, MaaResOption key, MaaString value);
    MaaSize MAA_API MaaResourceGetHash(MaaResourceHandle res, char* buff, MaaSize buff_size);

    /* Controller */

    MaaControllerHandle MAA_API MaaAdbControllerCreate(MaaString adb_path, MaaString address, MaaAdbControllerType type,
                                                       MaaJsonString config, MaaControllerCallback callback,
                                                       MaaCallbackTransparentArg callback_arg);
    MaaControllerHandle MAA_API MaaCustomControllerCreate(MaaCustomControllerHandle handle,
                                                          MaaControllerCallback callback,
                                                          MaaCallbackTransparentArg callback_arg);

    void MAA_API MaaControllerDestroy(MaaControllerHandle* ctrl);

    MaaBool MAA_API MaaControllerSetOption(MaaControllerHandle ctrl, MaaCtrlOption key, MaaString value);

    MaaCtrlId MAA_API MaaControllerPostConnection(MaaControllerHandle ctrl);
    MaaCtrlId MAA_API MaaControllerPostClick(MaaControllerHandle ctrl, int32_t x, int32_t y);
    MaaCtrlId MAA_API MaaControllerPostSwipe(MaaControllerHandle ctrl, int32_t* x_steps_buff, int32_t* y_steps_buff,
                                             int32_t* step_delay_buff, MaaSize buff_size);
    MaaCtrlId MAA_API MaaControllerPostScreencap(MaaControllerHandle ctrl);

    MaaStatus MAA_API MaaControllerStatus(MaaControllerHandle ctrl, MaaCtrlId id);
    MaaStatus MAA_API MaaControllerWait(MaaControllerHandle ctrl, MaaCtrlId id);
    MaaBool MAA_API MaaControllerConnected(MaaControllerHandle ctrl);

    MaaSize MAA_API MaaControllerGetImage(MaaControllerHandle ctrl, void* buff, MaaSize buff_size);
    MaaSize MAA_API MaaControllerGetUUID(MaaControllerHandle ctrl, char* buff, MaaSize buff_size);

    /* Instance */

    MaaInstanceHandle MAA_API MaaCreate(MaaInstanceCallback callback, MaaCallbackTransparentArg callback_arg);
    void MAA_API MaaDestroy(MaaInstanceHandle* inst);
    MaaBool MAA_API MaaSetOption(MaaInstanceHandle inst, MaaInstOption key, MaaString value);

    MaaBool MAA_API MaaBindResource(MaaInstanceHandle inst, MaaResourceHandle res);
    MaaBool MAA_API MaaBindController(MaaInstanceHandle inst, MaaControllerHandle ctrl);
    MaaBool MAA_API MaaInited(MaaInstanceHandle inst);

    MaaTaskId MAA_API MaaPostTask(MaaInstanceHandle inst, MaaString task, MaaJsonString param);
    MaaBool MAA_API MaaSetTaskParam(MaaInstanceHandle inst, MaaTaskId id, MaaJsonString param);

    MaaStatus MAA_API MaaTaskStatus(MaaInstanceHandle inst, MaaTaskId id);
    MaaStatus MAA_API MaaTaskWait(MaaInstanceHandle inst, MaaTaskId id);
    MaaBool MAA_API MaaTaskAllFinished(MaaInstanceHandle inst);

    void MAA_API MaaStop(MaaInstanceHandle inst);

    MaaSize MAA_API MaaGetResourceHash(MaaInstanceHandle inst, char* buff, MaaSize buff_size);
    MaaSize MAA_API MaaGetControllerUUID(MaaInstanceHandle inst, char* buff, MaaSize buff_size);

    /* Utils */

    MaaString MAA_API MaaVersion();

#ifdef __cplusplus
}
#endif
