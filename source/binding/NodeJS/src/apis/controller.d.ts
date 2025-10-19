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
            post_connection(): Job<CtrlId, Controller>
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
            config: string
        ]

        class AdbController extends Controller {
            constructor(
                adb_path: string,
                address: string,
                screencap_methods: ScreencapOrInputMethods,
                input_methods: ScreencapOrInputMethods,
                config: string,
                agent?: string
            )

            static agent_path(): string
            static find(adb?: string): Promise<AdbDevice[] | null>
        }
    }
}

export {}
