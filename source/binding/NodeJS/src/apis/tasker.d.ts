declare global {
    namespace maa {
        type TaskDetail = {
            entry: string
            nodes: NodeId[]
            status: Status
        }

        type NodeDetail = {
            name: string
            reco: RecoDetail | null
            action: ActionDetail | null
            completed: boolean
        }

        type RecoDetailEntry = {
            box: Rect
            score?: number // TemplateMatch
            count?: number // FeatureMatch | ColorMatch
            text?: string // OCR
            cls_index?: number // NeuralNetworkClassify | NeuralNetworkDetect
            label?: string // NeuralNetworkClassify | NeuralNetworkDetect
            detail?: string | Record<string, unknown> // Custom
        }

        type RecoDetailObject = {
            all: RecoDetailEntry[]
            filtered: RecoDetailEntry[]
            best: RecoDetailEntry | null
        }

        type RecoDetailWithoutDraws = {
            name: string
            algorithm: string
            hit: boolean
            box: Rect | null
            detail: RecoDetailObject | RecoDetailWithoutDraws[] | null
        }

        type RecoDetail = RecoDetailWithoutDraws & {
            raw: ArrayBuffer
            draws: ArrayBuffer[]
        }

        type ActionDetailObject = ActionParam

        type ActionDetail = {
            name: string
            action: string
            box: Rect
            success: boolean
            detail: ActionDetailObject
        }

        type TaskerNotify = {
            msg: NotifyMessage<'Task'>
            reason?: string
            task_id: number // TaskId
            entry: string
            uuid: string
            hash: string
        }

        type RuntimeNotify = {
            context_id?: number
            parent_task_id?: number
            task_parent?: { kind: string; id: number } | null
            parent?: { kind: string; id: number } | null
            reason?: string
        }

        type ContextTaskNotify = RuntimeNotify & {
            msg: NotifyMessage<'Context.Task'>
            task_id: number
            entry: string
        }

        type NodeTransitionNotify = RuntimeNotify & {
            msg: 'Transition'
            task_id: number
            from_node_id: number
            to_node_id: number
            from: string
            to: string
            reason: 'entry' | 'next' | 'on_error' | 'jump_back'
        }

        type TaskerContextPipelineNodeNotify = RuntimeNotify & {
            msg: NotifyMessage<'PipelineNode'>
            task_id: number // TaskId
            node_id: number // NodeId
            name: string
            focus: unknown
        }

        type TaskerContextRecognitionNodeNotify = RuntimeNotify & {
            msg: NotifyMessage<'RecognitionNode'>
            task_id: number // TaskId
            node_id: number // NodeId
            name: string
            focus: unknown
        }

        type TaskerContextActionNodeNotify = RuntimeNotify & {
            msg: NotifyMessage<'ActionNode'>
            task_id: number // TaskId
            node_id: number // NodeId
            name: string
            focus: unknown
        }

        type TaskerContextNextListNotify = RuntimeNotify & {
            msg: NotifyMessage<'NextList'>
            list_id?: number
            node_id?: number
            round?: number
            list_type?: 'entry' | 'next' | 'on_error'
            selected_index?: number
            reco_id?: number
            task_id: number // TaskId
            name: string
            list: {
                name: string
                jump_back: boolean
                anchor: boolean
            }[]
            focus: unknown
        }

        type TaskerContextRecognitionNotify = RuntimeNotify & {
            msg: NotifyMessage<'Recognition'>
            task_id: number // TaskId
            reco_id: number // RecoId
            name: string
            focus: unknown
            anchor?: string
            pipeline?: Record<string, unknown>
        }

        type TaskerContextActionNotify = RuntimeNotify & {
            msg: NotifyMessage<'Action'>
            reco_id?: number
            task_id: number // TaskId
            action_id: number // ActId
            name: string
            focus: unknown
            pipeline?: Record<string, unknown>
        }

        type TaskerContextWaitFreezesNotify = RuntimeNotify & {
            msg: NotifyMessage<'WaitFreezes'>
            task_id: number
            wf_id: number
            name: string
            phase: 'pre' | 'post' | 'repeat' | 'context'
            roi: Rect
            param: { time: number; threshold: number; method: number; rate_limit: number; timeout: number }
            reco_ids?: number[]
            elapsed?: number
            focus: unknown
        }

        type TaskerContextNotify =
            | TaskerContextWaitFreezesNotify
            | ContextTaskNotify
            | NodeTransitionNotify
            | TaskerContextPipelineNodeNotify
            | TaskerContextRecognitionNodeNotify
            | TaskerContextActionNodeNotify
            | TaskerContextNextListNotify
            | TaskerContextRecognitionNotify
            | TaskerContextActionNotify

        class TaskJob extends Job<TaskId, Tasker, TaskDetail> {}

        class Tasker {
            constructor(handle?: string)
            destroy(): void
            add_sink(cb: (tasker: Tasker, msg: TaskerNotify) => MaybePromise<void>): SinkId
            remove_sink(id: SinkId): void
            clear_sinks(): void
            add_context_sink(
                cb: (context: Context, msg: TaskerContextNotify) => MaybePromise<void>,
            ): SinkId
            remove_context_sink(id: SinkId): void
            clear_context_sinks(): void
            post_task(
                entry: string,
                pipeline_override?: Record<string, unknown> | Record<string, unknown>[],
            ): TaskJob
            post_recognition(
                reco_type: RecognitionType,
                reco_param: Record<string, unknown> | Record<string, unknown>[],
                image: ImageData,
            ): TaskJob
            /** Recognize one node only. node.completed is the effective hit after inverse. */
            post_recognition_node(
                entry: string,
                image: ImageData,
                pipeline_override?: Record<string, unknown> | Record<string, unknown>[],
            ): TaskJob
            post_action(
                action_type: ActionType,
                action_param: Record<string, unknown> | Record<string, unknown>[],
                box: maa.Rect,
                reco_detail?: string,
            ): TaskJob
            post_stop(): TaskJob
            status(id: TaskId): Status
            wait(id: TaskId): Promise<Status>
            get inited(): boolean
            get running(): boolean
            get stopping(): boolean
            set resource(res: Resource | null)
            get resource(): Resource | null
            set controller(res: Controller | null)
            get controller(): Controller | null
            /**
             * Clear historical details and latest-node references. Pending and running
             * tasks retain their IDs, entry and status, but their node ID lists are reset.
             */
            clear_cache(): void
            /** Instance settings; global defaults remain unchanged. */
            set reco_image_raw(value: boolean)
            set reco_image_draws(value: boolean)
            /** Unsigned decimal string, following the native uint64 binding. */
            set reco_image_cache_count(value: string)
            /** Unsigned decimal byte count. */
            set reco_image_cache_bytes(value: string)
            set capture_config(value: boolean)
            override_pipeline(
                task_id: TaskId,
                pipeline: Record<string, unknown> | Record<string, unknown>[],
            ): void
            recognition_image_status(id: RecoId):
                | 'available' | 'not_captured' | 'disabled' | 'budget_exceeded' | 'evicted' | null
            recognition_detail(id: RecoId): RecoDetail | null
            action_detail(id: ActId): ActionDetail | null
            node_detail(id: NodeId): NodeDetail | null
            task_detail(id: TaskId): TaskDetail | null
            latest_node(node_name: string): NodeId | null
        }
    }
}

export {}
