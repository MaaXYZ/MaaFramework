declare global {
    namespace maa {
        const Status: Record<'Invalid' | 'Pending' | 'Running' | 'Succeeded' | 'Failed', Status>
        const LoggingLevel: Record<
            'Off' | 'Fatal' | 'Error' | 'Warn' | 'Info' | 'Debug' | 'Trace' | 'All',
            LoggingLevel
        >
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
        const AdbInputMethod: Record<
            'AdbShell' | 'MinitouchAndAdbKey' | 'Maatouch' | 'EmulatorExtras' | 'All' | 'Default',
            ScreencapOrInputMethods
        >
        const Win32ScreencapMethod: Record<
            'GDI' | 'FramePool' | 'DXGI_DesktopDup',
            ScreencapOrInputMethods
        >
        const Win32InputMethod: Record<'Seize' | 'SendMessage', ScreencapOrInputMethods>
    }
}
export {}
