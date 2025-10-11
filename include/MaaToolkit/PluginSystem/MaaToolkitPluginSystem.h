// IWYU pragma: private, include <MaaToolkit/MaaToolkitAPI.h>

#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// load with full path or name only, name only will search in system directory and current directory
    MAA_TOOLKIT_API MaaBool MaaToolkitPluginSystemLoadLibrary( //
        const char* library_name,
        MaaTasker* tasker,
        MaaResource* resource,
        MaaController* controller);

    /// load with recursive search in the directory
    MAA_TOOLKIT_API MaaBool MaaToolkitPluginSystemLoadDirectory(
        const char* directory_path,
        MaaTasker* tasker,
        MaaResource* resource,
        MaaController* controller);

#ifdef __cplusplus
}
#endif
