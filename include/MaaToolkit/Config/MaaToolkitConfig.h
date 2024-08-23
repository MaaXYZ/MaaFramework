/**
 * @file MaaToolkitConfig.h
 * @author
 * @brief Init and uninit the toolkit.
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

    MAA_TOOLKIT_API MaaBool MaaToolkitConfigInitOption(const char* user_path, const char* default_json);

#ifdef __cplusplus
}
#endif
