declare global {
    namespace maa {
        type TaskDetail = {
            entry: string
            nodes: (NodeDetail | null)[]
            status: Status
        }

        type NodeDetail = {
            name: string
            reco: RecoDetail | null
            action: ActionDetail | null
            completed: boolean
        }

        type RecoDetailEntry = {
            box: maa.FlatRect
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
            box: Rect
            detail: RecoDetailObject
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
            msg: 'Task.Started' | 'Task.Completed' | 'Task.Failed'
            task_id: number // TaskId
            entry: string
            uuid: string
            hash: string
        }

        type TaskerContextNotify =
            | {
                  msg: 'NextList.Starting' | 'NextList.Succeeded' | 'NextList.Failed'
                  task_id: number // TaskId
                  name: string
                  list: string[]
                  focus: unknown
              }
            | {
                  msg: 'Recognition.Starting' | 'Recognition.Succeeded' | 'Recognition.Failed'
                  task_id: number // TaskId
                  reco_id: number // RecoId
                  name: string
                  focus: unknown
              }
            | {
                  msg: 'Action.Starting' | 'Action.Succeeded' | 'Action.Failed'
                  task_id: number // TaskId
                  node_id: number // NodeId
                  action_id: number // ActId
                  name: string
                  focus: unknown
              }

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
            clear_cache(): void
            recognition_detail(id: RecoId): RecoDetail | null
            action_detail(id: ActId): ActionDetail | null
            node_detail(id: NodeId): NodeDetail | null
            task_detail(id: TaskId): TaskDetail | null
            latest_node(node_name: string): NodeId | null
        }
    }
}

export {}
