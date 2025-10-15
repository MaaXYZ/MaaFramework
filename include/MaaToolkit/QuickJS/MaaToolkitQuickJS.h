/**
 * @file MaaToolkitQuickJS.h
 * @author
 * @brief Simple quickjs runtime.
 *
 * @copyright Copyright (c) 2024
 *
 */

// IWYU pragma: private, include <MaaToolkit/MaaToolkitAPI.h>

#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_TOOLKIT_API void MaaToolkitRunQuickJS(const char* script, MaaStringBuffer* output);

#ifdef __cplusplus
}
#endif
