#pragma once

#include "../MaaToolKitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaSize MAA_TOOLKIT_API MaaToolKitFindWindow(MaaStringView class_name, MaaStringView window_name);
    MaaSize MAA_TOOLKIT_API MaaToolKitSearchWindow(MaaStringView class_name, MaaStringView window_name);
    MaaWin32Hwnd MAA_TOOLKIT_API MaaToolKitGetWindow(MaaSize index);
    MaaWin32Hwnd MAA_TOOLKIT_API MaaToolKitGetCursorWindow();

#ifdef __cplusplus
}
#endif
