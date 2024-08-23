// IWYU pragma: private, include <MaaToolkit/MaaToolkitAPI.h>

#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_TOOLKIT_API MaaToolkitDesktopWindowList* MaaToolkitDesktopWindowListCreate();
    MAA_TOOLKIT_API void MaaToolkitDesktopWindowListDestroy(MaaToolkitDesktopWindowList* handle);

    MAA_TOOLKIT_API MaaBool MaaToolkitDesktopWindowFindAll(/*out*/ MaaToolkitDesktopWindowList* buffer);

    MAA_TOOLKIT_API MaaSize MaaToolkitDesktopWindowListSize(const MaaToolkitDesktopWindowList* list);
    MAA_TOOLKIT_API const MaaToolkitDesktopWindow* MaaToolkitDesktopWindowListAt(const MaaToolkitDesktopWindowList* list, MaaSize index);

    MAA_TOOLKIT_API void* MaaToolkitDesktopWindowGetHandle(const MaaToolkitDesktopWindow* window);
    MAA_TOOLKIT_API const char* MaaToolkitDesktopWindowGetClassName(const MaaToolkitDesktopWindow* window);
    MAA_TOOLKIT_API const char* MaaToolkitDesktopWindowGetWindowName(const MaaToolkitDesktopWindow* window);

#ifdef __cplusplus
}
#endif
