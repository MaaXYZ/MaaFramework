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
            ctrl_id: CtrlId
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
                input_methods: ScreencapOrInputMethods,
            )

            static find(): Promise<DesktopDevice[] | null>
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
        }

        class CustomController extends Controller {
            constructor(actor: CustomControllerActor)
        }
    }
}

export {}
