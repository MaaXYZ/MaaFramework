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

    MAA_TOOLKIT_API MaaBool
        MaaToolkitInitOptionConfig(MaaStringView user_path, MaaStringView default_json);

    /// \deprecated Use MaaToolkitInitOptionConfig instead.
    MAA_DEPRECATED MAA_TOOLKIT_API MaaBool MaaToolkitInit();
    /// \deprecated Don't use it.
    MAA_DEPRECATED MAA_TOOLKIT_API MaaBool MaaToolkitUninit();

#ifdef __cplusplus
}
#endif
