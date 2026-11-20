#pragma once

#include <stdint.h>

#include "MaaPluginPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_PLUGIN_API const char* GetPluginVersion(void);

    MAA_PLUGIN_API uint32_t GetApiVersion(void);

    // Optional ABI-v1 hooks. Initialization is deferred until first object creation.
    // Global management hooks must not create objects or reenter plugin management.
    MAA_PLUGIN_API void OnPluginInit(void);
    MAA_PLUGIN_API void OnPluginFlush(void);
    MAA_PLUGIN_API void OnPluginShutdown(void);

    struct MaaController;

    /// Optional pair for a plugin that provides a controller. The name must be nonempty and unique.
    MAA_PLUGIN_API const char* GetCustomControllerName(void);
    /// Return a newly owned MaaFramework controller, or NULL on failure. config_json is valid during this call.
    MAA_PLUGIN_API struct MaaController* CreateCustomController(const char* config_json);

    MAA_PLUGIN_API void OnResourceEvent(void* handle, const char* message, const char* details_json, void* trans_arg);
    MAA_PLUGIN_API void OnControllerEvent(void* handle, const char* message, const char* details_json, void* trans_arg);
    MAA_PLUGIN_API void OnTaskerEvent(void* handle, const char* message, const char* details_json, void* trans_arg);
    MAA_PLUGIN_API void OnContextEvent(void* handle, const char* message, const char* details_json, void* trans_arg);

#ifdef __cplusplus
}
#endif
