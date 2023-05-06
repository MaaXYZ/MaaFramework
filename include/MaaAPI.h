#pragma once

#include "MaaDef.h"
#include "MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /* Resource */

    MaaResourceHandle MAAAPI MaaResourceCreate(const char* path);
    MaaResourceHandle MAAAPI MaaResourceCreateEx(const char* path, MaaResourceCallback callback, void* custom_arg);
    void MAAAPI MaaResourceDestroy(MaaResourceHandle* res);
    MaaBool MAAAPI MaaResourceIncrementalLoad(const char* path);
    MaaBool MAAAPI MaaResourceSetOption(MaaResourceHandle ctrl, MaaResourceOptionKey key, const char* value);
    MaaBool MAAAPI MaaResourceLoading(MaaResourceHandle res);
    MaaBool MAAAPI MaaResourceLoaded(MaaResourceHandle res);
    MaaSize MAAAPI MaaGetHash(MaaResourceHandle handle, char* buff, MaaSize buff_size);

    /* Controller */

    MaaControllerHandle MAAAPI MaaControllerCreate(const char* adb_path, const char* address, const char* config_json);
    MaaControllerHandle MAAAPI MaaControllerCreateEx(const char* adb_path, const char* address, const char* config_json,
                                                     MaaControllerCallback callback, void* custom_arg);
    void MAAAPI MaaControllerDestroy(MaaControllerHandle* ctrl);
    MaaBool MAAAPI MaaControllerSetOption(MaaControllerHandle ctrl, MaaControllerOptionKey key, const char* value);
    MaaBool MAAAPI MaaControllerConnecting(MaaControllerHandle ctrl);
    MaaBool MAAAPI MaaControllerConnected(MaaControllerHandle ctrl);

    MaaCtrlId MAAAPI MaaClick(MaaControllerHandle ctrl, int32_t x, int32_t y, MaaBool block);
    MaaCtrlId MAAAPI MaaScreencap(MaaControllerHandle ctrl, MaaBool block);
    MaaSize MAAAPI MaaGetImage(MaaControllerHandle handle, void* buff, MaaSize buff_size);
    MaaSize MAAAPI MaaGetUUID(MaaControllerHandle handle, char* buff, MaaSize buff_size);

    /* Instance */

    MaaInstanceHandle MAAAPI MaaInstanceCreate(const char* user_path);
    MaaInstanceHandle MAAAPI MaaInstanceCreateEx(const char* user_path, MaaInstanceCallback callback, void* custom_arg);
    void MAAAPI MaaInstanceDestroy(MaaInstanceHandle* inst);
    MaaBool MAAAPI MaaInstanceSetOption(MaaInstanceHandle inst, MaaInstanceOptionKey key, const char* value);

    MaaBool MAAAPI MaaBindResource(MaaInstanceHandle inst, MaaResourceHandle res);
    MaaBool MAAAPI MaaBindController(MaaInstanceHandle inst, MaaControllerHandle ctrl);
    MaaBool MAAAPI MaaInstanceInited(MaaInstanceHandle ctrl);

    MaaTaskId MAAAPI MaaAppendTask(MaaInstanceHandle handle, const char* type, const char* param);
    MaaBool MAAAPI MaaSetTaskParam(MaaInstanceHandle handle, MaaTaskId id, const char* param);

    MaaBool MAAAPI MaaStart(MaaInstanceHandle handle);
    MaaBool MAAAPI MaaStop(MaaInstanceHandle handle);
    MaaBool MAAAPI MaaRunning(MaaInstanceHandle handle);

    MaaSize MAAAPI MaaGetResourceHash(MaaInstanceHandle handle, char* buff, MaaSize buff_size);
    MaaSize MAAAPI MaaGetControllerUUID(MaaInstanceHandle handle, char* buff, MaaSize buff_size);
    MaaSize MAAAPI MaaGetTasksList(MaaInstanceHandle handle, MaaTaskId* buff, MaaSize buff_size);

    /* Utils */

    MAAAPI_PORT const char* MAA_CALL MaaVersion();
    MaaBool MAAAPI MaaSetStaticOption(MaaStaticOptionKey key, const char* value);
    MaaSize MAAAPI MaaGetNullSize();

#ifdef __cplusplus
}
#endif
