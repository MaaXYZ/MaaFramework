#pragma once

#include "MaaPluginPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_PLUGIN_API void OnTaskerMessage(const char* message, const char* details_json, void* notify_trans_arg);
    MAA_PLUGIN_API void OnResourceMessage(const char* message, const char* details_json, void* notify_trans_arg);
    MAA_PLUGIN_API void OnControllerMessage(const char* message, const char* details_json, void* notify_trans_arg);

#ifdef __cplusplus
}
#endif
