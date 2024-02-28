/**
 * @file MaaToolkitConfig.h
 * @author
 * @brief Init and uninit the toolkit.
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaBool MAA_TOOLKIT_API MaaToolkitInit();
    MaaBool MAA_TOOLKIT_API MaaToolkitUninit();

#ifdef __cplusplus
}
#endif
