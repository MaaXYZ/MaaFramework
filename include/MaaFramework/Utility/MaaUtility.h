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

    MAA_FRAMEWORK_API const char* MaaVersion();

    MAA_FRAMEWORK_API MaaBool MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size);

#ifdef __cplusplus
}
#endif
