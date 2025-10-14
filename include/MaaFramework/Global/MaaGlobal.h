/**
 * @file MaaUtility.h
 * @author
 * @brief Provide global functions for the framework.
 *
 * @copyright Copyright (c) 2024
 *
 */

// IWYU pragma: private, include <MaaFramework/MaaAPI.h>

#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @param[in] value
     */
    MAA_FRAMEWORK_API MaaBool
        MaaGlobalSetOption(MaaGlobalOption key, MaaOptionValue value /**< byte array, int*, char*, bool* */, MaaOptionValueSize val_size);

    /// load a plugin with full path or name only, name only will search in system directory and current directory
    /// or
    /// load plugins with recursive search in the directory
    MAA_FRAMEWORK_API MaaBool MaaGlobalLoadPlugin(const char* library_path);

    MAA_DEPRECATED MAA_FRAMEWORK_API MaaBool
        MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value /**< byte array, int*, char*, bool* */, MaaOptionValueSize val_size);

#ifdef __cplusplus
}
#endif
