export type AgentClientHandle = { __brand: 'AgentClientHandle' }
export type ResourceHandle = { __brand: 'ResourceHandle' }
export type ControllerHandle = { __brand: 'ControllerHandle' }
export type TaskerHandle = { __brand: 'TaskerHandle' }
export type ContextHandle = { __brand: 'ContextHandle' }
export type DesktopHandle = string & { __brand: 'DesktopHandle' }

type Uint64 = number | string
type Id = Uint64
type ScreencapOrInputMethods = Uint64

export type ResId = Id & { __brand: 'ResId' }
export type CtrlId = Id & { __brand: 'CtrlId' }
export type TaskId = Id & { __brand: 'TaskId' }
export type RecoId = Id & { __brand: 'RecoId' }
export type NodeId = Id & { __brand: 'NodeId' }

export type Status = number & { __brand: 'Status' }
export type LoggingLevel = number & { __brand: 'LoggingLevel' }
export type InferenceDevice = number & { __brand: 'InferenceDevice' }
export type InferenceExecutionProvider = number & { __brand: 'InferenceExecutionProvider' }

export type ImageData = ArrayBuffer

export type Rect = {
    x: number
    y: number
    width: number
    height: number
}

export type FlatRect = [x: number, y: number, width: number, height: number]

type MaybePromise<T> = T | Promise<T>

export type NotificationCallback = (message: string, details_json: string) => MaybePromise<void>
export type CustomRecognitionCallback = (
    context: ContextHandle,
    task_id: TaskId,
    node_name: string,
    custom_recognition_name: string,
    custom_recognition_param: string,
    image: ImageData,
    roi: Rect
) => MaybePromise<[out_box: Rect, out_detail: string] | null>
export type CustomActionCallback = (
    context: ContextHandle,
    task_id: TaskId,
    node_name: string,
    custom_action_name: string,
    custom_action_param: string,
    reco_id: RecoId,
    box: Rect
) => MaybePromise<boolean>

export type CustomControllerParamResultMap = {
    connect: [[], boolean]
    request_uuid: [[], string | null]
    start_app: [[intent: string], boolean]
    stop_app: [[intent: string], boolean]
    screencap: [[], ImageData | null]
    click: [[x: number, y: number], boolean]
    swipe: [[x1: number, y1: number, x2: number, y2: number, duration: number], boolean]
    touch_down: [[contact: number, x: number, y: number, pressure: number], boolean]
    touch_move: [[contact: number, x: number, y: number, pressure: number], boolean]
    touch_up: [[contact: number], boolean]
    press_key: [[keycode: number], boolean]
    input_text: [[text: string], boolean]
}
export type CustomControllerCallback = (
    action: keyof CustomControllerParamResultMap,
    ...param: any[]
) => MaybePromise<any>

// context.cpp

export declare function context_run_task(
    context: ContextHandle,
    entry: string,
    pipeline_override: string
): Promise<TaskId>
export declare function context_run_recognition(
    context: ContextHandle,
    entry: string,
    pipeline_override: string,
    image: ImageData
): Promise<RecoId>
export declare function context_run_action(
    context: ContextHandle,
    entry: string,
    pipeline_override: string,
    box: Rect,
    reco_detail: string
): Promise<NodeId>
export declare function context_override_pipeline(
    context: ContextHandle,
    pipeline_override: string
): boolean
export declare function context_override_next(
    context: ContextHandle,
    node_name: string,
    next: string[]
): boolean
export declare function context_get_node_data(
    context: ContextHandle,
    node_name: string
): string | null
export declare function context_get_task_id(context: ContextHandle): TaskId
export declare function context_get_tasker(context: ContextHandle): TaskerHandle
export declare function context_clone(context: ContextHandle): ContextHandle

// controller.cpp

export declare function adb_controller_create(
    adb_path: string,
    address: string,
    screencap_methods: ScreencapOrInputMethods,
    input_methods: ScreencapOrInputMethods,
    config: string,
    agent_path: string,
    callback: NotificationCallback | null
): ControllerHandle | null
export declare function win32_controller_create(
    handle: DesktopHandle,
    screencap_methods: ScreencapOrInputMethods,
    input_methods: ScreencapOrInputMethods,
    callback: NotificationCallback | null
): ControllerHandle | null
export declare function custom_controller_create(
    custom_callback: CustomControllerCallback,
    callback: NotificationCallback | null
): ControllerHandle | null
export declare function dbg_controller_create(
    read_path: string,
    write_path: string,
    type: Uint64,
    config: string,
    callback: NotificationCallback | null
): ControllerHandle | null
export declare function controller_destroy(handle: ControllerHandle): void
export declare function controller_set_option_screenshot_target_long_side(
    handle: ControllerHandle,
    value: number
): boolean
export declare function controller_set_option_screenshot_target_short_side(
    handle: ControllerHandle,
    value: number
): boolean
export declare function controller_set_option_screenshot_use_raw_size(
    handle: ControllerHandle,
    value: boolean
): boolean
export declare function controller_set_option_recording(
    handle: ControllerHandle,
    value: boolean
): boolean
export declare function controller_post_connection(handle: ControllerHandle): CtrlId
export declare function controller_post_click(
    handle: ControllerHandle,
    x: number,
    y: number
): CtrlId
export declare function controller_post_swipe(
    handle: ControllerHandle,
    x1: number,
    y1: number,
    x2: number,
    y2: number,
    duration: number
): CtrlId
export declare function controller_post_press_key(handle: ControllerHandle, keycode: number): CtrlId
export declare function controller_post_input_text(handle: ControllerHandle, text: string): CtrlId
export declare function controller_post_start_app(handle: ControllerHandle, intent: string): CtrlId
export declare function controller_post_stop_app(handle: ControllerHandle, intent: string): CtrlId
export declare function controller_post_touch_down(
    handle: ControllerHandle,
    contact: number,
    x: number,
    y: number,
    pressure: number
): CtrlId
export declare function controller_post_touch_move(
    handle: ControllerHandle,
    contact: number,
    x: number,
    y: number,
    pressure: number
): CtrlId
export declare function controller_post_touch_up(handle: ControllerHandle, contact: number): CtrlId
export declare function controller_post_screencap(handle: ControllerHandle): CtrlId
export declare function controller_status(handle: ControllerHandle, ctrl_id: CtrlId): Status
export declare function controller_wait(handle: ControllerHandle, ctrl_id: CtrlId): Promise<Status>
export declare function controller_connected(handle: ControllerHandle): boolean
export declare function controller_cached_image(handle: ControllerHandle): ImageData | null
export declare function controller_get_uuid(handle: ControllerHandle): string | null

// resource.cpp

export declare function resource_create(
    callback: NotificationCallback | null
): ResourceHandle | null
export declare function resource_destroy(handle: ResourceHandle): void
export declare function resource_set_option_inference_device(
    handle: ResourceHandle,
    id: InferenceDevice | number
): boolean
export declare function resource_set_option_inference_execution_provider(
    handle: ResourceHandle,
    provider: InferenceExecutionProvider
): boolean
export declare function resource_register_custom_recognition(
    handle: ResourceHandle,
    name: string,
    recognizer: CustomRecognitionCallback
): boolean
export declare function resource_unregister_custom_recognition(
    handle: ResourceHandle,
    name: string
): boolean
export declare function resource_clear_custom_recognition(handle: ResourceHandle): boolean
export declare function resource_register_custom_action(
    handle: ResourceHandle,
    name: string,
    action: CustomActionCallback
): boolean
export declare function resource_unregister_custom_action(
    handle: ResourceHandle,
    name: string
): boolean
export declare function resource_clear_custom_action(handle: ResourceHandle): boolean
export declare function resource_post_bundle(handle: ResourceHandle, path: string): ResId
export declare function resource_override_pipeline(
    handle: ResourceHandle,
    pipeline_override: string
): bool
export declare function resource_override_next(
    handle: ResourceHandle,
    node_name: string,
    next_list: string[]
): bool
export declare function resource_get_node_data(
    handle: ResourceHandle,
    node_name: string
): string | null
export declare function resource_clear(handle: ResourceHandle): boolean
export declare function resource_status(handle: ResourceHandle, res_id: ResId): Status
export declare function resource_wait(handle: ResourceHandle, res_id: ResId): Promise<Status>
export declare function resource_loaded(handle: ResourceHandle): boolean
export declare function resource_get_hash(handle: ResourceHandle): string | null
export declare function resource_get_node_list(handle: ResourceHandle): string[] | null

// tasker.cpp

export declare function tasker_create(callback: NotificationCallback | null): TaskerHandle | null
export declare function tasker_destroy(handle: TaskerHandle): void
export declare function tasker_bind_resource(
    handle: TaskerHandle,
    resource: ResourceHandle | null
): boolean
export declare function tasker_bind_controller(
    handle: TaskerHandle,
    controller: ControllerHandle | null
): boolean
export declare function tasker_inited(handle: TaskerHandle): boolean
export declare function tasker_post_task(
    handle: TaskerHandle,
    entry: string,
    pipeline_override: string
): TaskId
export declare function tasker_status(handle: TaskerHandle, task_id: TaskId): Status
export declare function tasker_wait(handle: TaskerHandle, task_id: TaskId): Promise<Status>
export declare function tasker_running(handle: TaskerHandle): boolean
export declare function tasker_post_stop(handle: TaskerHandle): TaskId
export declare function tasker_stopping(handle: TaskerHandle): boolean
export declare function tasker_get_resource(handle: TaskerHandle): ResourceHandle | null
export declare function tasker_get_controller(handle: TaskerHandle): ControllerHandle | null
export declare function tasker_clear_cache(handle: TaskerHandle): boolean
export declare function tasker_get_recognition_detail(
    handle: TaskerHandle,
    reco_id: RecoId
):
    | [
          name: string,
          algorithm: string,
          hit: boolean,
          box: Rect,
          detail: string,
          raw: ImageData,
          draws: ImageData[]
      ]
    | null
export declare function tasker_get_node_detail(
    handle: TaskerHandle,
    node_id: NodeId
): [name: string, reco_id: RecoId, completed: boolean] | null
export declare function tasker_get_task_detail(
    handle: TaskerHandle,
    task_id: TaskId
): [entry: string, node_ids: NodeId[], status: Status] | null
export declare function tasker_get_latest_node(
    handle: TaskerHandle,
    node_name: string
): NodeId | null

// config.cpp

export declare function config_init_option(user_path: string, default_json: string): boolean

// find.cpp

export declare function find_adb(
    adb_path: string | null
): Promise<
    | [
          name: string,
          adb_path: string,
          address: string,
          screencap_methods: ScreencapOrInputMethods,
          input_methods: ScreencapOrInputMethods,
          config: string
      ][]
    | null
>
export declare function find_desktop(): Promise<
    [handle: DesktopHandle, class_name: string, window_name: string][] | null
>

// utility.cpp

export declare function version(): string
export declare function set_global_option_log_dir(value: string): boolean
export declare function set_global_option_save_draw(value: boolean): boolean
export declare function set_global_option_recording(value: boolean): boolean
export declare function set_global_option_stdout_level(value: LoggingLevel): boolean
export declare function set_global_option_show_hit_draw(value: boolean): boolean
export declare function set_global_option_debug_mode(value: boolean): boolean

// pi.cpp

export declare function pi_register_custom_recognizer(
    id: Id,
    name: string,
    recognizer: CustomRecognitionCallback
): void
export declare function pi_register_custom_action(
    id: Id,
    name: string,
    action: CustomActionCallback
): void
export declare function pi_run_cli(
    id: Id,
    resource_path: string,
    user_path: string,
    directly: boolean,
    callback: NotificationCallback | null
): Promise<boolean>

export declare const Status: Record<
    'Invalid' | 'Pending' | 'Running' | 'Succeeded' | 'Failed',
    Status
>
export declare const LoggingLevel: Record<
    'Off' | 'Fatal' | 'Error' | 'Warn' | 'Info' | 'Debug' | 'Trace' | 'All',
    LoggingLevel
>
export declare const AdbScreencapMethod: Record<
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
export declare const AdbInputMethod: Record<
    'AdbShell' | 'MinitouchAndAdbKey' | 'Maatouch' | 'EmulatorExtras' | 'All' | 'Default',
    ScreencapOrInputMethods
>
export declare const Win32ScreencapMethod: Record<
    'GDI' | 'FramePool' | 'DXGI_DesktopDup',
    ScreencapOrInputMethods
>
export declare const Win32InputMethod: Record<'Seize' | 'SendMessage', ScreencapOrInputMethods>
export declare const DbgControllerType: Record<'CarouselImage' | 'ReplayRecording', Uint64>
export declare const InferenceDevice: Record<'CPU' | 'Auto', InferenceDevice>
export declare const InferenceExecutionProvider: Record<
    'Auto' | 'CPU' | 'DirectML' | 'CoreML' | 'CUDA',
    InferenceExecutionProvider
>

export declare const AgentRole: 'client' | 'server'

// agent.cpp - client

export declare function agent_client_create(identifier: string | null): AgentClientHandle | null
export declare function agent_client_destroy(handle: AgentClientHandle): void
export declare function agent_client_identifier(handle: AgentClientHandle): string | null
export declare function agent_client_bind_resource(
    handle: AgentClientHandle,
    resource: ResourceHandle
): boolean
export declare function agent_client_connect(handle: AgentClientHandle): Promise<boolean>
export declare function agent_client_disconnect(handle: AgentClientHandle): boolean
export declare function agent_client_connected(handle: AgentClientHandle): boolean
export declare function agent_client_alive(handle: AgentClientHandle): boolean
export declare function agent_client_set_timeout(handle: AgentClientHandle, ms: Uint64): boolean

// agent.cpp - server

export declare function agent_server_register_custom_recognition(
    name: string,
    recognizer: CustomRecognitionCallback
): boolean
export declare function agent_server_register_custom_action(
    name: string,
    action: CustomActionCallback
): boolean
export declare function agent_server_start_up(identifier: string): boolean
export declare function agent_server_shut_down(): void
export declare function agent_server_join(): Promise<true>
export declare function agent_server_detach(): void
