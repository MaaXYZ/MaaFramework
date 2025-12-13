declare global {
    namespace maa {
        const Status: Record<'Invalid' | 'Pending' | 'Running' | 'Succeeded' | 'Failed', Status>
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
            | 'GDI'
            | 'FramePool'
            | 'DXGI_DesktopDup'
            | 'DXGI_DesktopDup_Window'
            | 'PrintWindow'
            | 'ScreenDC',
            ScreencapOrInputMethods
        >
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
        const RecognitionType: Record<
            | 'DirectHit'
            | 'TemplateMatch'
            | 'FeatureMatch'
            | 'ColorMatch'
            | 'OCR'
            | 'NeuralNetworkClassify'
            | 'NeuralNetworkDetect'
            | 'Custom',
            string
        >
        const ActionType: Record<
            | 'DoNothing'
            | 'Click'
            | 'LongPress'
            | 'Swipe'
            | 'MultiSwipe'
            | 'TouchDown'
            | 'TouchMove'
            | 'TouchUp'
            | 'ClickKey'
            | 'LongPressKey'
            | 'KeyDown'
            | 'KeyUp'
            | 'InputText'
            | 'StartApp'
            | 'StopApp'
            | 'StopTask'
            | 'Scroll'
            | 'Command'
            | 'Custom',
            string
        >
    }
}
export {}
