declare global {
    namespace maa {
        type ClickParam = {
            point: Point
            contact: number
        }

        type LongPressParam = {
            point: Point
            duration: number
            contact: number
        }

        type SwipeParam = {
            begin: Point
            end: Point[]
            end_hold: number[]
            duration: number[]
            only_hover: boolean
            starting: number
            contact: number
        }

        type MultiSwipeParam = {
            swipes: SwipeParam[]
        }

        type TouchParam = {
            contact: number
            point: Point
            pressure: number
        }

        type ClickKeyParam = {
            keycode: number[]
        }

        type LongPressKeyParam = {
            keycode: number[]
            duration: number
        }

        type InputTextParam = {
            text: string
        }

        type AppParam = {
            package: string
        }

        type ScrollParam = {
            dx: number
            dy: number
        }

        type ActionParam =
            | {}
            | ClickParam
            | LongPressParam
            | SwipeParam
            | MultiSwipeParam
            | TouchParam
            | ClickKeyParam
            | LongPressKeyParam
            | InputTextParam
            | AppParam
            | ScrollParam

        type ControllerNotify = {
            msg: NotifyMessage<'Action'>
            ctrl_id: number // CtrlId
            uuid: string
        } & (
            | {
                  action: 'connect'
                  param?: never
              }
            | {
                  action: 'click'
                  param: ClickParam
              }
            | {
                  action: 'long_press'
                  param: LongPressParam
              }
            | {
                  action: 'swipe'
                  param: SwipeParam
              }
            | {
                  action: 'multi_swipe'
                  param: MultiSwipeParam
              }
            | {
                  action: 'touch_down' | 'touch_move' | 'touch_up'
                  param: TouchParam
              }
            | {
                  action: 'click_key' | 'key_down' | 'key_up'
                  param: ClickKeyParam
              }
            | {
                  action: 'long_press_key'
                  param: LongPressKeyParam
              }
            | {
                  action: 'input_text'
                  param: InputTextParam
              }
            | {
                  action: 'screencap'
                  param?: never
              }
            | {
                  action: 'start_app' | 'stop_app'
                  param: AppParam
              }
            | {
                  action: 'scroll'
                  param: ScrollParam
              }
        )

        class ImageJob extends Job<CtrlId, Controller, ImageData | null> {
            constructor(source: Controller, id: CtrlId)
        }

        class Controller {
            constructor(handle?: string)

            destroy(): void
            add_sink(cb: (ctrl: Controller, msg: ControllerNotify) => MaybePromise<void>): SinkId
            remove_sink(id: SinkId): void
            clear_sinks(): void

            set screenshot_target_long_side(value: number)
            set screenshot_target_short_side(value: number)
            set screenshot_use_raw_size(value: boolean)

            post_connection(): Job<CtrlId, Controller>
            post_click(x: number, y: number): Job<CtrlId, Controller>
            post_swipe(
                x1: number,
                y1: number,
                x2: number,
                y2: number,
                duration: number,
            ): Job<CtrlId, Controller>
            post_click_key(keycode: number): Job<CtrlId, Controller>
            post_input_text(text: string): Job<CtrlId, Controller>
            post_start_app(intent: string): Job<CtrlId, Controller>
            post_stop_app(intent: string): Job<CtrlId, Controller>
            post_touch_down(
                contact: number,
                x: number,
                y: number,
                pressure: number,
            ): Job<CtrlId, Controller>
            post_touch_move(
                contact: number,
                x: number,
                y: number,
                pressure: number,
            ): Job<CtrlId, Controller>
            post_touch_up(contact: number): Job<CtrlId, Controller>
            post_key_down(keycode: number): Job<CtrlId, Controller>
            post_key_up(keycode: number): Job<CtrlId, Controller>
            /**
             * Post a scroll action. Using multiples of 120 (WHEEL_DELTA) is recommended for best compatibility.
             */
            post_scroll(dx: number, dy: number): Job<CtrlId, Controller>
            post_screencap(): ImageJob
            override_pipeline(pipeline: Record<string, unknown> | Record<string, unknown>[]): void
            override_next(node_name: string, next_list: string[]): void
            get_node_data(node_name: string): string | null
            get_node_data_parsed(node_name: string): DumpTask | null
            clear(): void
            status(id: CtrlId): Status
            wait(id: CtrlId): Promise<Status>
            get connected(): boolean
            get cached_image(): ImageData | null
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

        class AdbController extends Controller {
            constructor(
                adb_path: string,
                address: string,
                screencap_methods: ScreencapOrInputMethods,
                input_methods: ScreencapOrInputMethods,
                config: string,
                agent?: string,
            )

            static agent_path(): string
            static find(adb?: string): Promise<AdbDevice[] | null>
        }

        type DesktopDevice = [handle: DesktopHandle, class_name: string, window_name: string]

        class Win32Controller extends Controller {
            constructor(
                hwnd: DesktopHandle,
                screencap_methods: ScreencapOrInputMethods,
                mouse_method: ScreencapOrInputMethods,
                keyboard_methods: ScreencapOrInputMethods,
            )

            static find(): Promise<DesktopDevice[] | null>
        }

        class PlayCoverController extends Controller {
            constructor(address: string, uuid: string)
        }

        class DbgController extends Controller {
            constructor(
                read_path: string,
                write_path: string,
                type: Uint64, // DbgControllerType
                config: string,
            )
        }

        interface CustomControllerActor {
            connect?(): maa.MaybePromise<boolean>
            request_uuid?(): maa.MaybePromise<string | null>
            get_features?(): maa.MaybePromise<null | ('mouse' | 'keyboard')[]>
            start_app?(intent: string): maa.MaybePromise<boolean>
            stop_app?(intent: string): maa.MaybePromise<boolean>
            screencap?(): maa.MaybePromise<maa.ImageData | null>
            click?(x: number, y: number): maa.MaybePromise<boolean>
            swipe?(
                x1: number,
                y1: number,
                x2: number,
                y2: number,
                duration: number,
            ): maa.MaybePromise<boolean>
            touch_down?(
                contact: number,
                x: number,
                y: number,
                pressure: number,
            ): maa.MaybePromise<boolean>
            touch_move?(
                contact: number,
                x: number,
                y: number,
                pressure: number,
            ): maa.MaybePromise<boolean>
            touch_up?(contact: number): maa.MaybePromise<boolean>
            click_key?(keycode: number): maa.MaybePromise<boolean>
            input_text?(text: string): maa.MaybePromise<boolean>
            key_down?(keycode: number): maa.MaybePromise<boolean>
            key_up?(keycode: number): maa.MaybePromise<boolean>
            scroll?(dx: number, dy: number): maa.MaybePromise<boolean>
        }

        class CustomController extends Controller {
            constructor(actor: CustomControllerActor)
        }
    }
}

export {}
