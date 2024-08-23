// IWYU pragma: private, include <MaaToolkit/MaaToolkitAPI.h>

#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_TOOLKIT_API MaaToolkitDesktopWindowList* MaaToolkitDesktopWindowCreateList();
    MAA_TOOLKIT_API void MaaToolkitDesktopWindowDestroyList(MaaToolkitDesktopWindowList* handle);

    MAA_TOOLKIT_API MaaBool MaaToolkitDesktopWindowGetAll(/*out*/ MaaToolkitDesktopWindowList* buffer);

    MAA_TOOLKIT_API MaaSize MaaToolkitDesktopWindowListSize(MaaToolkitDesktopWindowList* list);
    MAA_TOOLKIT_API MaaToolkitDesktopWindow* MaaToolkitDesktopWindowListAt(MaaToolkitDesktopWindowList* list, MaaSize index);

    MAA_TOOLKIT_API void* MaaToolkitDesktopWindowGetHandle(MaaToolkitDesktopWindow* window);
    MAA_TOOLKIT_API const char* MaaToolkitDesktopWindowGetClassName(MaaToolkitDesktopWindow* window);
    MAA_TOOLKIT_API const char* MaaToolkitDesktopWindowGetWindowName(MaaToolkitDesktopWindow* window);

#ifdef __cplusplus
}
#endif
