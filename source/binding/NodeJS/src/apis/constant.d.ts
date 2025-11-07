declare global {
    namespace maa {
        /**
         * 状态常量 / Status constants
         * 
         * - Invalid: 无效 / Invalid
         * - Pending: 等待中 / Pending
         * - Running: 运行中 / Running
         * - Succeeded: 成功 / Succeeded
         * - Failed: 失败 / Failed
         */
        const Status: Record<'Invalid' | 'Pending' | 'Running' | 'Succeeded' | 'Failed', Status>
        /**
         * Adb 截图方式常量 / Adb screenshot method constants
         * 
         * - EncodeToFileAndPull: 编码到文件并拉取 / Encode to file and pull
         * - Encode: 编码 / Encode
         * - RawWithGzip: 原始数据（使用 Gzip） / Raw data (with Gzip)
         * - RawByNetcat: 原始数据（使用 Netcat） / Raw data (with Netcat)
         * - MinicapDirect: Minicap 直接模式 / Minicap direct mode
         * - MinicapStream: Minicap 流模式 / Minicap stream mode
         * - EmulatorExtras: 模拟器扩展 / Emulator extras
         * - All: 所有方式 / All methods
         * - Default: 默认方式 / Default method
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
         * Adb 输入方式常量 / Adb input method constants
         * 
         * - AdbShell: Adb Shell 输入 / Adb Shell input
         * - MinitouchAndAdbKey: Minitouch 和 Adb 按键 / Minitouch and Adb key
         * - Maatouch: Maatouch 输入 / Maatouch input
         * - EmulatorExtras: 模拟器扩展 / Emulator extras
         * - All: 所有方式 / All methods
         * - Default: 默认方式 / Default method
         */
        const AdbInputMethod: Record<
            'AdbShell' | 'MinitouchAndAdbKey' | 'Maatouch' | 'EmulatorExtras' | 'All' | 'Default',
            ScreencapOrInputMethods
        >
        /**
         * Win32 截图方式常量 / Win32 screenshot method constants
         * 
         * - GDI: GDI 截图 / GDI screenshot
         * - FramePool: 帧池截图 / Frame pool screenshot
         * - DXGI_DesktopDup: DXGI 桌面复制 / DXGI desktop duplication
         */
        const Win32ScreencapMethod: Record<
            'GDI' | 'FramePool' | 'DXGI_DesktopDup',
            ScreencapOrInputMethods
        >
        /**
         * Win32 输入方式常量 / Win32 input method constants
         * 
         * - Seize: 捕获输入 / Seize input
         * - SendMessage: 发送消息 / Send message
         */
        const Win32InputMethod: Record<'Seize' | 'SendMessage', ScreencapOrInputMethods>
        /**
         * 调试控制器类型常量 / Debug controller type constants
         * 
         * - CarouselImage: 轮播图片模式 / Carousel image mode
         * - ReplayRecording: 重放录制模式 / Replay recording mode
         */
        const DbgControllerType: Record<'CarouselImage' | 'ReplayRecording', Uint64>
    }
}
export {}
