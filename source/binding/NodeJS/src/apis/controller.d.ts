declare global {
    namespace maa {
        type Point = [x: number, y: number]

        type SwipeParam = {
            begin: Point
            end: Point[]
            end_hold: number[]
            duration: number[]
            only_hover: boolean
            starting: number
        }

        type ControllerNotify = {
            msg:
                | 'Controller.Action.Starting'
                | 'Controller.Action.Succeeded'
                | 'Controller.Action.Failed'
            ctrl_id: number // CtrlId
            uuid: string
        } & (
            | {
                  action: 'connect'
                  param?: never
              }
            | {
                  action: 'click'
                  param: {
                      point: Point
                  }
              }
            | {
                  action: 'long_press'
                  param: {
                      point: Point
                      duration: number
                  }
              }
            | {
                  action: 'swipe'
                  param: SwipeParam
              }
            | {
                  action: 'multi_swipe'
                  param: SwipeParam[]
              }
            | {
                  action: 'touch_down' | 'touch_move' | 'touch_up'
                  param: {
                      contact: number
                      point: Point
                      pressure: number
                  }
              }
            | {
                  action: 'click_key' | 'key_down' | 'key_up'
                  param: {
                      keycode: number[]
                  }
              }
            | {
                  action: 'long_press_key'
                  param: {
                      keycode: number[]
                      duration: number
                  }
              }
            | {
                  action: 'input_text'
                  param: {
                      text: string
                  }
              }
            | {
                  action: 'screencap'
                  param?: never
              }
            | {
                  action: 'start_app' | 'stop_app'
                  param: {
                      package: string
                  }
              }
        )

        /**
         * 图片作业类 / Image job class
         */
        class ImageJob extends Job<CtrlId, Controller, ImageData | null> {
            /**
             * @param source 控制器源 / Controller source
             * @param id 控制器 id / Controller id
             */
            constructor(source: Controller, id: CtrlId)
        }

        /**
         * 控制器类 / Controller class
         */
        class Controller {
            /**
             * 创建控制器 / Create controller
             * @param handle 可选的外部句柄 / Optional external handle
             */
            constructor(handle?: string)

            /** 销毁控制器 / Destroy controller */
            destroy(): void
            /**
             * 添加控制器事件监听器 / Add controller event listener
             * @param cb 回调函数 / Callback function
             * @returns 监听器 id / Listener id
             */
            add_sink(cb: (ctrl: Controller, msg: ControllerNotify) => MaybePromise<void>): SinkId
            /**
             * 移除控制器事件监听器 / Remove controller event listener
             * @param id 监听器 id / Listener id
             */
            remove_sink(id: SinkId): void
            /** 清除所有控制器事件监听器 / Clear all controller event listeners */
            clear_sinks(): void

            /**
             * 设置截图缩放长边到指定长度 / Set screenshot scaling long side to specified length
             * @param value 长边长度 / Long side length
             */
            set screenshot_target_long_side(value: number)
            /**
             * 设置截图缩放短边到指定长度 / Set screenshot scaling short side to specified length
             * @param value 短边长度 / Short side length
             */
            set screenshot_target_short_side(value: number)
            /**
             * 设置截图不缩放 / Set screenshot use raw size without scaling
             * 注意：此选项可能导致在不同分辨率的设备上坐标不正确
             * Note: This option may cause incorrect coordinates on devices with different resolutions
             * @param value 是否启用 / Whether to enable
             */
            set screenshot_use_raw_size(value: boolean)

            /**
             * 连接设备 / Connect device
             * @returns 作业对象 / Job object
             */
            post_connection(): Job<CtrlId, Controller>
            /**
             * 点击 / Click
             * @param x x 坐标 / x coordinate
             * @param y y 坐标 / y coordinate
             * @returns 作业对象 / Job object
             */
            post_click(x: number, y: number): Job<CtrlId, Controller>
            /**
             * 滑动 / Swipe
             * @param x1 起点 x 坐标 / Start x coordinate
             * @param y1 起点 y 坐标 / Start y coordinate
             * @param x2 终点 x 坐标 / End x coordinate
             * @param y2 终点 y 坐标 / End y coordinate
             * @param duration 滑动时长(毫秒) / Swipe duration in milliseconds
             * @returns 作业对象 / Job object
             */
            post_swipe(
                x1: number,
                y1: number,
                x2: number,
                y2: number,
                duration: number,
            ): Job<CtrlId, Controller>
            /**
             * 单击按键 / Click key
             * @param keycode 虚拟键码 / Virtual key code
             * @returns 作业对象 / Job object
             */
            post_click_key(keycode: number): Job<CtrlId, Controller>
            /**
             * 输入文本 / Input text
             * @param text 要输入的文本 / Text to input
             * @returns 作业对象 / Job object
             */
            post_input_text(text: string): Job<CtrlId, Controller>
            /**
             * 启动应用 / Start app
             * @param intent 目标应用 (Adb 控制器: package name 或 activity) / Target app (Adb controller: package name or activity)
             * @returns 作业对象 / Job object
             */
            post_start_app(intent: string): Job<CtrlId, Controller>
            /**
             * 关闭应用 / Stop app
             * @param intent 目标应用 (Adb 控制器: package name) / Target app (Adb controller: package name)
             * @returns 作业对象 / Job object
             */
            post_stop_app(intent: string): Job<CtrlId, Controller>
            /**
             * 按下 / Touch down
             * @param contact 触点编号 (Adb 控制器: 手指编号; Win32 控制器: 鼠标按键 0:左键, 1:右键, 2:中键) / Contact number (Adb controller: finger number; Win32 controller: mouse button 0:left, 1:right, 2:middle)
             * @param x x 坐标 / x coordinate
             * @param y y 坐标 / y coordinate
             * @param pressure 触点力度 / Contact pressure
             * @returns 作业对象 / Job object
             */
            post_touch_down(
                contact: number,
                x: number,
                y: number,
                pressure: number,
            ): Job<CtrlId, Controller>
            /**
             * 移动 / Move
             * @param contact 触点编号 (Adb 控制器: 手指编号; Win32 控制器: 鼠标按键 0:左键, 1:右键, 2:中键) / Contact number (Adb controller: finger number; Win32 controller: mouse button 0:left, 1:right, 2:middle)
             * @param x x 坐标 / x coordinate
             * @param y y 坐标 / y coordinate
             * @param pressure 触点力度 / Contact pressure
             * @returns 作业对象 / Job object
             */
            post_touch_move(
                contact: number,
                x: number,
                y: number,
                pressure: number,
            ): Job<CtrlId, Controller>
            /**
             * 抬起 / Touch up
             * @param contact 触点编号 (Adb 控制器: 手指编号; Win32 控制器: 鼠标按键 0:左键, 1:右键, 2:中键) / Contact number (Adb controller: finger number; Win32 controller: mouse button 0:left, 1:right, 2:middle)
             * @returns 作业对象 / Job object
             */
            post_touch_up(contact: number): Job<CtrlId, Controller>
            /**
             * 按下键 / Key down
             * @param keycode 虚拟键码 / Virtual key code
             * @returns 作业对象 / Job object
             */
            post_key_down(keycode: number): Job<CtrlId, Controller>
            /**
             * 抬起键 / Key up
             * @param keycode 虚拟键码 / Virtual key code
             * @returns 作业对象 / Job object
             */
            post_key_up(keycode: number): Job<CtrlId, Controller>
            /**
             * 截图 / Screenshot
             * @returns 图片作业对象，可通过 result 获取截图 / Image job object, can get screenshot via result
             */
            post_screencap(): ImageJob
            /**
             * 覆盖 pipeline / Override pipeline
             * @param pipeline 用于覆盖的 json / JSON for overriding
             */
            override_pipeline(pipeline: Record<string, unknown> | Record<string, unknown>[]): void
            /**
             * 覆盖任务的 next 列表 / Override the next list of task
             * @param node_name 任务名 / Task name
             * @param next_list next 列表 / Next list
             */
            override_next(node_name: string, next_list: string[]): void
            /**
             * 获取任务当前的定义 / Get the current definition of task
             * @param node_name 任务名 / Task name
             * @returns 任务定义字符串，如果不存在则返回 null / Task definition string, or null if not exists
             */
            get_node_data(node_name: string): string | null
            /**
             * 获取任务当前的定义（已解析） / Get the current definition of task (parsed)
             * @param node_name 任务名 / Task name
             * @returns 任务定义对象，如果不存在则返回 null / Task definition object, or null if not exists
             */
            get_node_data_parsed(node_name: string): DumpTask | null
            /** 清除已加载内容 / Clear loaded content */
            clear(): void
            /**
             * 查询操作状态 / Query status of operation
             * @param id 操作 id / Operation id
             * @returns 状态 / Status
             */
            status(id: CtrlId): Status
            /**
             * 等待操作完成 / Wait for operation to complete
             * @param id 操作 id / Operation id
             * @returns 状态 / Status
             */
            wait(id: CtrlId): Promise<Status>
            /** 判断是否已连接 / Check if connected */
            get connected(): boolean
            /** 获取最新一次截图 / Get the latest screenshot */
            get cached_image(): ImageData | null
            /** 获取设备 uuid / Get device uuid */
            get uuid(): string | null
        }

        type AdbDevice = [
            name: string,
            adb_path: string,
            address: string,
            screencap_methods: ScreencapOrInputMethods,
            input_methods: ScreencapOrInputMethods,
            config: string,
        ]

        /**
         * Adb 控制器 / Adb controller
         * 
         * 截图方式和输入方式会在启动时进行测速, 选择最快的方案
         * Screenshot and input methods will be speed tested at startup, selecting the fastest option
         */
        class AdbController extends Controller {
            /**
             * 创建 Adb 控制器 / Create Adb controller
             * @param adb_path adb 路径 / adb path
             * @param address 连接地址 / connection address
             * @param screencap_methods 所有可使用的截图方式 / all available screenshot methods
             * @param input_methods 所有可使用的输入方式 / all available input methods
             * @param config 额外配置 / extra config
             * @param agent MaaAgentBinary 路径 / MaaAgentBinary path
             */
            constructor(
                adb_path: string,
                address: string,
                screencap_methods: ScreencapOrInputMethods,
                input_methods: ScreencapOrInputMethods,
                config: string,
                agent?: string,
            )

            /**
             * 获取 Agent 二进制路径 / Get agent binary path
             * @returns Agent 路径 / Agent path
             */
            static agent_path(): string
            /**
             * 搜索所有已知安卓模拟器 / Search all known Android emulators
             * @param adb 可选，指定 adb 路径进行搜索 / Optional, search using specified adb path
             * @returns 设备列表，如果失败则返回 null / Device list, or null if failed
             */
            static find(adb?: string): Promise<AdbDevice[] | null>
        }

        type DesktopDevice = [handle: DesktopHandle, class_name: string, window_name: string]

        /**
         * Win32 控制器 / Win32 controller
         */
        class Win32Controller extends Controller {
            /**
             * 创建 Win32 控制器 / Create Win32 controller
             * @param hwnd 窗口句柄 / window handle
             * @param screencap_methods 使用的截图方式 / screenshot method used
             * @param mouse_method 使用的鼠标输入方式 / mouse input method used
             * @param keyboard_methods 使用的键盘输入方式 / keyboard input method used
             */
            constructor(
                hwnd: DesktopHandle,
                screencap_methods: ScreencapOrInputMethods,
                mouse_method: ScreencapOrInputMethods,
                keyboard_methods: ScreencapOrInputMethods,
            )

            /**
             * 查询所有窗口信息 / Query all window info
             * @returns 窗口列表，如果失败则返回 null / Window list, or null if failed
             */
            static find(): Promise<DesktopDevice[] | null>
        }

        /**
         * 调试控制器 / Debug controller
         */
        class DbgController extends Controller {
            /**
             * 创建调试控制器 / Create debug controller
             * @param read_path 输入路径, 包含通过 Recording 选项记录的操作 / Input path, includes operations recorded via Recording option
             * @param write_path 输出路径, 包含执行结果 / Output path, includes execution results
             * @param type 控制器模式 / Controller mode
             * @param config 额外配置 / Extra config
             */
            constructor(
                read_path: string,
                write_path: string,
                type: Uint64, // DbgControllerType
                config: string,
            )
        }

        /**
         * 自定义控制器接口 / Custom controller interface
         * 
         * 不需要实现所有函数，只需实现需要的函数即可
         * You do not have to implement all the functions, just implement the functions you need
         */
        interface CustomControllerActor {
            /** 连接 / Connect */
            connect?(): maa.MaybePromise<boolean>
            /** 请求 uuid / Request uuid */
            request_uuid?(): maa.MaybePromise<string | null>
            /** 获取特性 / Get features */
            get_features?(): maa.MaybePromise<null | ('mouse' | 'keyboard')[]>
            /**
             * 启动应用 / Start app
             * @param intent 目标应用 / Target app
             */
            start_app?(intent: string): maa.MaybePromise<boolean>
            /**
             * 关闭应用 / Stop app
             * @param intent 目标应用 / Target app
             */
            stop_app?(intent: string): maa.MaybePromise<boolean>
            /** 截图 / Screenshot */
            screencap?(): maa.MaybePromise<maa.ImageData | null>
            /**
             * 点击 / Click
             * @param x x 坐标 / x coordinate
             * @param y y 坐标 / y coordinate
             */
            click?(x: number, y: number): maa.MaybePromise<boolean>
            /**
             * 滑动 / Swipe
             * @param x1 起点 x 坐标 / Start x coordinate
             * @param y1 起点 y 坐标 / Start y coordinate
             * @param x2 终点 x 坐标 / End x coordinate
             * @param y2 终点 y 坐标 / End y coordinate
             * @param duration 滑动时长(毫秒) / Swipe duration in milliseconds
             */
            swipe?(
                x1: number,
                y1: number,
                x2: number,
                y2: number,
                duration: number,
            ): maa.MaybePromise<boolean>
            /**
             * 按下 / Touch down
             * @param contact 触点编号 / Contact number
             * @param x x 坐标 / x coordinate
             * @param y y 坐标 / y coordinate
             * @param pressure 触点力度 / Contact pressure
             */
            touch_down?(
                contact: number,
                x: number,
                y: number,
                pressure: number,
            ): maa.MaybePromise<boolean>
            /**
             * 移动 / Move
             * @param contact 触点编号 / Contact number
             * @param x x 坐标 / x coordinate
             * @param y y 坐标 / y coordinate
             * @param pressure 触点力度 / Contact pressure
             */
            touch_move?(
                contact: number,
                x: number,
                y: number,
                pressure: number,
            ): maa.MaybePromise<boolean>
            /**
             * 抬起 / Touch up
             * @param contact 触点编号 / Contact number
             */
            touch_up?(contact: number): maa.MaybePromise<boolean>
            /**
             * 单击按键 / Click key
             * @param keycode 虚拟键码 / Virtual key code
             */
            click_key?(keycode: number): maa.MaybePromise<boolean>
            /**
             * 输入文本 / Input text
             * @param text 要输入的文本 / Text to input
             */
            input_text?(text: string): maa.MaybePromise<boolean>
            /**
             * 按下键 / Key down
             * @param keycode 虚拟键码 / Virtual key code
             */
            key_down?(keycode: number): maa.MaybePromise<boolean>
            /**
             * 抬起键 / Key up
             * @param keycode 虚拟键码 / Virtual key code
             */
            key_up?(keycode: number): maa.MaybePromise<boolean>
        }

        /**
         * 自定义控制器 / Custom controller
         */
        class CustomController extends Controller {
            /**
             * 创建自定义控制器 / Create custom controller
             * @param actor 自定义控制器接口实现 / Custom controller interface implementation
             */
            constructor(actor: CustomControllerActor)
        }
    }
}

export {}
