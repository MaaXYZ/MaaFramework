#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaSize MAA_TOOLKIT_API MaaToolkitFindWindow(MaaStringView class_name, MaaStringView window_name);
    MaaSize MAA_TOOLKIT_API MaaToolkitSearchWindow(MaaStringView class_name, MaaStringView window_name);
    MaaWin32Hwnd MAA_TOOLKIT_API MaaToolkitGetWindow(MaaSize index);
    MaaWin32Hwnd MAA_TOOLKIT_API MaaToolkitGetCursorWindow();

#ifdef __cplusplus
}
#endif
