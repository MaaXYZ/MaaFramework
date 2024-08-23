/**
 * @file MaaCustomAction.h
 * @author
 * @brief Custom action API.
 *
 * @copyright Copyright (c) 2024
 *
 */

// IWYU pragma: private, include <MaaFramework/MaaAPI.h>

#pragma once

#include "../MaaDef.h"

#ifdef MAA_BUILD_PLUGIN
#define MAA_PLUGIN_INTERFACE MAA_DLL_EXPORT
#else
#define MAA_PLUGIN_INTERFACE
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    enum MaaPluginHookVersionEnum
    {
        MaaPluginHookVersion_V0 = 0,
    };

    typedef int32_t MaaPluginHookVersion;

    struct MaaPluginHookDispatcher
    {
        MaaPluginHookVersion version;
    };

    struct MaaPluginHook_V0
    {
        MaaPluginHookVersion version;
        void (*on_instance_created)(MaaInstanceHandle inst);
        void (*on_instance_destroyed)(MaaInstanceHandle inst);
    };

    MAA_PLUGIN_INTERFACE int maafw_plugin_init(MaaPluginHookDispatcher* hook);

#ifdef __cplusplus
}
#endif
