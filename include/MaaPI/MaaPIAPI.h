#pragma once

#include "MaaPIDef.h" // IWYU pragma: export

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_PI_API MaaPIData* MaaPIDataCreate();
    MAA_PI_API void MaaPIDataDestroy(MaaPIData* data);
    MAA_PI_API MaaBool MaaPIDataLoad(MaaPIData* data, const char* json, const char* path);

    MAA_PI_API MaaPIConfig* MaaPIConfigCreate();
    MAA_PI_API void MaaPIConfigDestroy(MaaPIConfig* cfg);
    MAA_PI_API MaaBool MaaPIConfigLoad(MaaPIConfig* cfg, MaaPIData* data, const char* json);
    MAA_PI_API MaaBool MaaPIConfigGenDef(MaaPIConfig* cfg, MaaPIData* data);
    MAA_PI_API MaaBool MaaPIConfigSave(MaaPIConfig* cfg, MaaStringBuffer* json);

    MAA_PI_API MaaPIRuntime* MaaPIRuntimeCreate();
    MAA_PI_API void MaaPIRuntimeDestroy(MaaPIRuntime* rt);
    MAA_PI_API MaaController* MaaPIRuntimeGetController(MaaPIRuntime* rt);
    MAA_PI_API MaaResource* MaaPIRuntimeGetResource(MaaPIRuntime* rt);
    MAA_PI_API MaaTasker* MaaPIRuntimeGetTasker(MaaPIRuntime* rt);
    MAA_PI_API MaaBool MaaPIRuntimeBind(MaaPIRuntime* rt, MaaPIData* data, MaaPIConfig* cfg);
    MAA_PI_API MaaBool MaaPIRuntimeSetup(MaaPIRuntime* rt, MaaNotificationCallback cb, void* cb_arg);

    MAA_PI_API MaaPIClient* MaaPIClientCreate(const char* locale, MaaPIClientHandler handler, void* handler_arg);
    MAA_PI_API void MaaPIClientDestroy(MaaPIClient* client);
    MAA_PI_API MaaBool MaaPIClientPerform(MaaPIClient* client, MaaPIRuntime* rt, MaaPIClientAction action);

#ifdef __cplusplus
}
#endif

