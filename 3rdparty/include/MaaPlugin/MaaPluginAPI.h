#pragma once

#include <cstdint>

#include "MaaPluginPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_PLUGIN_API uint32_t GetApiVersion();

    MAA_PLUGIN_API void OnResourceEvent(void* handle, const char* message, const char* details_json, void* trans_arg);
    MAA_PLUGIN_API void OnControllerEvent(void* handle, const char* message, const char* details_json, void* trans_arg);
    MAA_PLUGIN_API void OnTaskerEvent(void* handle, const char* message, const char* details_json, void* trans_arg);
    MAA_PLUGIN_API void OnContextEvent(void* handle, const char* message, const char* details_json, void* trans_arg);

#ifdef __cplusplus
}
#endif
