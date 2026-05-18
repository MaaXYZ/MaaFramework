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

    // 通用字段
    MAA_TOOLKIT_API uint64_t MaaToolkitDesktopWindowGetHandle(const MaaToolkitDesktopWindow* window);
    MAA_TOOLKIT_API const char* MaaToolkitDesktopWindowGetWindowName(const MaaToolkitDesktopWindow* window);

    // Win32 专有字段
    MAA_TOOLKIT_API const char* MaaToolkitDesktopWindowGetWin32ClassName(const MaaToolkitDesktopWindow* window);

    // MacOS 专有字段
    MAA_TOOLKIT_API int32_t MaaToolkitDesktopWindowGetMacOSPID(const MaaToolkitDesktopWindow* window);                 // macOS 进程ID
    MAA_TOOLKIT_API const char* MaaToolkitDesktopWindowGetMacOSBundleID(const MaaToolkitDesktopWindow* window);        // macOS Bundle ID
    MAA_TOOLKIT_API const char* MaaToolkitDesktopWindowGetMacOSApplicationName(const MaaToolkitDesktopWindow* window); // macOS 应用程序名称

    // Linux 专有字段
    MAA_TOOLKIT_API const char* MaaToolkitDesktopWindowGetLinuxSocketPath(const MaaToolkitDesktopWindow* window);

#ifdef __cplusplus
}
#endif
