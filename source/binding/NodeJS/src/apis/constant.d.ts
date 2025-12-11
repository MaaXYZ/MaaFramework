declare global {
    namespace maa {
        const Status: Record<'Invalid' | 'Pending' | 'Running' | 'Succeeded' | 'Failed', Status>

        /**
         * Adb screencap method flags.
         *
         * Use bitwise OR to set the methods you need.
         * MaaFramework will test all provided methods and use the fastest available one.
         *
         * Default: All methods except RawByNetcat, MinicapDirect, MinicapStream
         *
         * Note: MinicapDirect and MinicapStream use lossy JPEG encoding, which may
         * significantly reduce template matching accuracy. Not recommended.
         *
         * | Method                | Speed      | Compatibility | Encoding | Notes                             |
         * |-----------------------|------------|---------------|----------|-----------------------------------|
         * | EncodeToFileAndPull   | Slow       | High          | Lossless |                                   |
         * | Encode                | Slow       | High          | Lossless |                                   |
         * | RawWithGzip           | Medium     | High          | Lossless |                                   |
         * | RawByNetcat           | Fast       | Low           | Lossless |                                   |
         * | MinicapDirect         | Fast       | Low           | Lossy    |                                   |
         * | MinicapStream         | Very Fast  | Low           | Lossy    |                                   |
         * | EmulatorExtras        | Very Fast  | Low           | Lossless | Emulators only: MuMu 12, LDPlayer 9 |
         */
        const AdbScreencapMethod: Record<
            | 'EncodeToFileAndPull'
            | 'Encode'
            | 'RawWithGzip'
            | 'RawByNetcat'
            | 'MinicapDirect'
            | 'MinicapStream'
            | 'EmulatorExtras'
            | 'All'
            | 'Default',
            ScreencapOrInputMethods
        >

        /**
         * Adb input method flags.
         *
         * Use bitwise OR to set the methods you need.
         * MaaFramework will select the first available method according to priority.
         *
         * Priority (high to low): EmulatorExtras > Maatouch > MinitouchAndAdbKey > AdbShell
         *
         * Default: All methods except EmulatorExtras
         *
         * | Method               | Speed | Compatibility | Notes                                 |
         * |----------------------|-------|---------------|---------------------------------------|
         * | AdbShell             | Slow  | High          |                                       |
         * | MinitouchAndAdbKey   | Fast  | Medium        | Key press still uses AdbShell         |
         * | Maatouch             | Fast  | Medium        |                                       |
         * | EmulatorExtras       | Fast  | Low           | Emulators only: MuMu 12               |
         */
        const AdbInputMethod: Record<
            'AdbShell' | 'MinitouchAndAdbKey' | 'Maatouch' | 'EmulatorExtras' | 'All' | 'Default',
            ScreencapOrInputMethods
        >

        /**
         * Win32 screencap method.
         *
         * No bitwise OR, select ONE method only.
         *
         * No default value. Client should choose one as default.
         *
         * Different applications use different rendering methods, there is no universal solution.
         *
         * | Method                  | Speed     | Compatibility | Require Admin | Background Support | Notes                            |
         * |-------------------------|-----------|---------------|---------------|--------------------|----------------------------------|
         * | GDI                     | Fast      | Medium        | No            | No                 |                                  |
         * | FramePool               | Very Fast | Medium        | No            | Yes                | Requires Windows 10 1903+        |
         * | DXGI_DesktopDup         | Very Fast | Low           | No            | No                 | Desktop duplication (full screen)|
         * | DXGI_DesktopDup_Window  | Very Fast | Low           | No            | No                 | Desktop duplication then crop    |
         * | PrintWindow             | Medium    | Medium        | No            | Yes                |                                  |
         * | ScreenDC                | Fast      | High          | No            | No                 |                                  |
         *
         * Note: When a window is minimized on Windows, all screencap methods will fail.
         * Avoid minimizing the target window.
         */
        const Win32ScreencapMethod: Record<
            | 'GDI'
            | 'FramePool'
            | 'DXGI_DesktopDup'
            | 'DXGI_DesktopDup_Window'
            | 'PrintWindow'
            | 'ScreenDC',
            ScreencapOrInputMethods
        >

        /**
         * Win32 input method.
         *
         * No bitwise OR, select ONE method only.
         *
         * No default value. Client should choose one as default.
         *
         * Different applications process input differently, there is no universal solution.
         *
         * | Method                       | Compatibility | Require Admin | Seize Mouse | Background Support | Notes                                                       |
         * |------------------------------|---------------|---------------|--------------|--------------------|-------------------------------------------------------------|
         * | Seize                        | High          | No            | Yes          | No                 |                                                             |
         * | SendMessage                  | Medium        | Maybe         | No           | Yes                |                                                             |
         * | PostMessage                  | Medium        | Maybe         | No           | Yes                |                                                             |
         * | LegacyEvent                  | Low           | No            | Yes          | No                 |                                                             |
         * | PostThreadMessage            | Low           | Maybe         | No           | Yes                |                                                             |
         * | SendMessageWithCursorPos     | Medium        | Maybe         | Briefly      | Yes                | Designed for apps that check real cursor position           |
         * | PostMessageWithCursorPos     | Medium        | Maybe         | Briefly      | Yes                | Designed for apps that check real cursor position           |
         *
         * Note:
         * - Admin rights mainly depend on the target application's privilege level.
         *   If the target runs as admin, MaaFramework should also run as admin for compatibility.
         * - "WithCursorPos" methods briefly move the cursor to target position, send message,
         *   then restore cursor position. This "briefly" seizes the mouse but won't block user operations.
         */
        const Win32InputMethod: Record<
            | 'Seize'
            | 'SendMessage'
            | 'PostMessage'
            | 'LegacyEvent'
            | 'PostThreadMessage'
            | 'SendMessageWithCursorPos'
            | 'PostMessageWithCursorPos'
            | 'Gamepad',
            ScreencapOrInputMethods
        >
        const DbgControllerType: Record<'CarouselImage' | 'ReplayRecording', Uint64>
    }
}
export {}
