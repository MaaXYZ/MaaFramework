#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Find a win32 window by class name and window name.
     *
     * This function finds the function by exact match. See also MaaToolkitSearchWindow().
     *
     * @param class_name The class name of the window. If passed an empty string, class name will
     * not be filtered.
     * @param window_name The window name of the window. If passed an empty string, window name will
     * not be filtered.
     * @return MaaSize The number of windows found that match the criteria. To get the corresponding
     * window handle, use MaaToolkitGetWindow().
     */
    MAA_TOOLKIT_API MaaSize
        MaaToolkitFindWindow(MaaStringView class_name, MaaStringView window_name);

    /**
     * @brief Search a win32 window by class name and window name.
     *
     * This function searches the function by substring match. See also MaaToolkitFindWindow().
     *
     * @param class_name The class name of the window. If passed an empty string, class name will
     * not be filtered.
     * @param window_name The window name of the window. If passed an empty string, window name will
     * not be filtered.
     * @return MaaSize The number of windows found that match the criteria. To get the corresponding
     * window handle, use MaaToolkitGetWindow().
     */
    MAA_TOOLKIT_API MaaSize
        MaaToolkitSearchWindow(MaaStringView class_name, MaaStringView window_name);

    /**
     * @brief Get the window handle by index.
     *
     * @param index The 0-based index of the window. The index should not exceed the number of
     * windows found otherwise out_of_range exception will be thrown.
     * @return MaaWin32Hwnd The window handle.
     */
    MAA_TOOLKIT_API MaaWin32Hwnd MaaToolkitGetWindow(MaaSize index);

    /**
     * @brief Get the window handle of the window under the cursor. This uses the WindowFromPoint()
     * system API.
     *
     * @return MaaWin32Hwnd The window handle.
     */
    MAA_TOOLKIT_API MaaWin32Hwnd MaaToolkitGetCursorWindow();

    /**
     * @brief Get the desktop window handle. This uses the GetDesktopWindow() system API.
     *
     * @return MaaWin32Hwnd The window handle.
     */
    MAA_TOOLKIT_API MaaWin32Hwnd MaaToolkitGetDesktopWindow();

    /**
     * @brief Get the foreground window handle. This uses the GetForegroundWindow() system API.
     *
     * @return MaaWin32Hwnd The window handle.
     */
    MAA_TOOLKIT_API MaaWin32Hwnd MaaToolkitGetForegroundWindow();

#ifdef __cplusplus
}
#endif
