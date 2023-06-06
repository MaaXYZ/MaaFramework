#pragma once

#include "MaaDef.h"
#include "MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif
    MaaBool MAAAPI MaaSetGlobalOption(MaaString key, MaaString value);

    /* Resource */

    MaaResourceHandle MAAAPI MaaResourceCreate(MaaString user_path, MaaResourceCallback callback, void* callback_arg);
    void MAAAPI MaaResourceDestroy(MaaResourceHandle* res);

    MaaResId MAAAPI MaaResourcePostLoad(MaaResourceHandle res, MaaString path);
    MaaStatus MAAAPI MaaResourceStatus(MaaResourceHandle res, MaaResId id);
    MaaBool MAAAPI MaaResourceLoaded(MaaResourceHandle res);

    MaaBool MAAAPI MaaResourceSetOption(MaaResourceHandle res, MaaString key, MaaString value);
    MaaSize MAAAPI MaaResourceGetHash(MaaResourceHandle res, char* buff, MaaSize buff_size);

    /* Controller */

    MaaControllerHandle MAAAPI MaaAdbControllerCreate(MaaString adb_path, MaaString address,
                                                      MaaControllerCallback callback, void* callback_arg);
    MaaControllerHandle MAAAPI MaaMinitouchControllerCreate(MaaString adb_path, MaaString address,
                                                            MaaControllerCallback callback, void* callback_arg);
    MaaControllerHandle MAAAPI MaaMaatouchControllerCreate(MaaString adb_path, MaaString address,
                                                           MaaControllerCallback callback, void* callback_arg);

    MaaControllerHandle MAAAPI MaaCustomControllerCreate(MaaCustomControllerHandle handle,
                                                         MaaControllerCallback callback, void* callback_arg);

    void MAAAPI MaaControllerDestroy(MaaControllerHandle* ctrl);

    MaaBool MAAAPI MaaControllerSetOption(MaaControllerHandle ctrl, MaaString key, MaaString value);

    MaaCtrlId MAAAPI MaaControllerPostConnect(MaaControllerHandle ctrl);
    MaaCtrlId MAAAPI MaaControllerPostClick(MaaControllerHandle ctrl, int32_t x, int32_t y);
    MaaCtrlId MAAAPI MaaControllerPostSwipe(MaaControllerHandle ctrl, int32_t* x_steps_buff, int32_t* y_steps_buff,
                                            int32_t* step_delay_buff, MaaSize buff_size);
    MaaCtrlId MAAAPI MaaControllerPostScreencap(MaaControllerHandle ctrl);

    MaaStatus MAAAPI MaaControllerStatus(MaaControllerHandle ctrl, MaaCtrlId id);
    MaaBool MAAAPI MaaControllerConnected(MaaControllerHandle ctrl);

    MaaSize MAAAPI MaaControllerGetImage(MaaControllerHandle ctrl, void* buff, MaaSize buff_size);
    MaaSize MAAAPI MaaControllerGetUUID(MaaControllerHandle ctrl, char* buff, MaaSize buff_size);

    /* Instance */

    MaaInstanceHandle MAAAPI MaaCreate(MaaInstanceCallback callback, void* callback_arg);
    void MAAAPI MaaDestroy(MaaInstanceHandle* inst);
    MaaBool MAAAPI MaaSetOption(MaaInstanceHandle inst, MaaString key, MaaString value);

    MaaBool MAAAPI MaaBindResource(MaaInstanceHandle inst, MaaResourceHandle res);
    MaaBool MAAAPI MaaBindController(MaaInstanceHandle inst, MaaControllerHandle ctrl);
    MaaBool MAAAPI MaaInited(MaaInstanceHandle inst);

    MaaTaskId MAAAPI MaaPostTask(MaaInstanceHandle inst, MaaString type, MaaString param);
    MaaBool MAAAPI MaaSetTaskParam(MaaInstanceHandle inst, MaaTaskId id, MaaString param);

    MaaStatus MAAAPI MaaTaskStatus(MaaInstanceHandle inst, MaaTaskId id);
    MaaBool MAAAPI MaaTaskAllFinished(MaaInstanceHandle inst);

    void MAAAPI MaaStop(MaaInstanceHandle inst);

    MaaSize MAAAPI MaaGetResourceHash(MaaInstanceHandle inst, char* buff, MaaSize buff_size);
    MaaSize MAAAPI MaaGetControllerUUID(MaaInstanceHandle inst, char* buff, MaaSize buff_size);

    /* Utils */

    MaaString MAAAPI MaaVersion();

#ifdef __cplusplus
}
#endif
