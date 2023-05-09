#pragma once

#include "MaaDef.h"
#include "MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif
    MaaBool MAAAPI MaaSetGlobalOption(const char* key, const char* value);

    /* Resource */

    MaaResourceHandle MAAAPI MaaResourceCreate(const char* path, const char* user_path, MaaResourceCallback callback,
                                               void* callback_arg);
    void MAAAPI MaaResourceDestroy(MaaResourceHandle* res);
    MaaBool MAAAPI MaaResourceIncrementalLoad(MaaResourceHandle res, const char* path);
    MaaBool MAAAPI MaaResourceSetOption(MaaResourceHandle res, const char* key, const char* value);
    MaaBool MAAAPI MaaResourceLoading(MaaResourceHandle res);
    MaaBool MAAAPI MaaResourceLoaded(MaaResourceHandle res);
    MaaSize MAAAPI MaaResourceGetHash(MaaResourceHandle res, char* buff, MaaSize buff_size);

    /* Controller */

    MaaControllerHandle MAAAPI MaaAdbControllerCreate(const char* adb_path, const char* address,
                                                      const char* config_json, MaaControllerCallback callback,
                                                      void* callback_arg);
    MaaControllerHandle MAAAPI MaaMinitouchControllerCreate(const char* adb_path, const char* address,
                                                            const char* config_json, MaaControllerCallback callback,
                                                            void* callback_arg);
    MaaControllerHandle MAAAPI MaaMaatouchControllerCreate(const char* adb_path, const char* address,
                                                           const char* config_json, MaaControllerCallback callback,
                                                           void* callback_arg);

    // FIXME: 不知道要什么参数，等 hg 佬改改测测（
    MaaControllerHandle MAAAPI MaaPlayToolsControllerCreate(const char* config_json, MaaControllerCallback callback,
                                                            void* callback_arg);

    void MAAAPI MaaControllerDestroy(MaaControllerHandle* ctrl);

    MaaBool MAAAPI MaaControllerSetOption(MaaControllerHandle ctrl, const char* key, const char* value);
    MaaBool MAAAPI MaaControllerConnecting(MaaControllerHandle ctrl);
    MaaBool MAAAPI MaaControllerConnected(MaaControllerHandle ctrl);

    MaaCtrlId MAAAPI MaaControllerClick(MaaControllerHandle ctrl, int32_t x, int32_t y);
    MaaCtrlId MAAAPI MaaControllerSwipe(MaaControllerHandle ctrl, int32_t* x_steps_buff, int32_t* y_steps_buff,
                                        int32_t* step_delay_buff, MaaSize buff_size);
    MaaCtrlId MAAAPI MaaControllerScreencap(MaaControllerHandle ctrl);
    MaaSize MAAAPI MaaControllerGetImage(MaaControllerHandle ctrl, void* buff, MaaSize buff_size);
    MaaSize MAAAPI MaaControllerGetUUID(MaaControllerHandle ctrl, char* buff, MaaSize buff_size);

    /* Instance */

    MaaInstanceHandle MAAAPI MaaCreate(MaaInstanceCallback callback, void* callback_arg);
    void MAAAPI MaaDestroy(MaaInstanceHandle* inst);
    MaaBool MAAAPI MaaSetOption(MaaInstanceHandle inst, const char* key, const char* value);

    MaaBool MAAAPI MaaBindResource(MaaInstanceHandle inst, MaaResourceHandle res);
    MaaBool MAAAPI MaaBindController(MaaInstanceHandle inst, MaaControllerHandle ctrl);
    MaaBool MAAAPI MaaInited(MaaInstanceHandle inst);

    MaaTaskId MAAAPI MaaAppendTask(MaaInstanceHandle inst, const char* type, const char* param);
    MaaBool MAAAPI MaaSetTaskParam(MaaInstanceHandle inst, MaaTaskId id, const char* param);

    MaaBool MAAAPI MaaStart(MaaInstanceHandle inst);
    MaaBool MAAAPI MaaStop(MaaInstanceHandle inst);
    MaaBool MAAAPI MaaRunning(MaaInstanceHandle inst);

    MaaSize MAAAPI MaaGetResourceHash(MaaInstanceHandle inst, char* buff, MaaSize buff_size);
    MaaSize MAAAPI MaaGetControllerUUID(MaaInstanceHandle inst, char* buff, MaaSize buff_size);
    MaaSize MAAAPI MaaGetTaskList(MaaInstanceHandle inst, MaaTaskId* buff, MaaSize buff_size);

    /* Utils */

    MAAAPI_PORT const char* MAA_CALL MaaVersion();
    // MaaBool MAAAPI MaaSetStaticOption(MaaStaticOptionKey key, const char* value);

#ifdef __cplusplus
}
#endif
