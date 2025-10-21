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

        type RecoDetail = {
            all: RecoDetailEntry[]
            filtered: RecoDetailEntry[]
            best: RecoDetailEntry | null
        }

        type TaskerNotify = {
            msg: 'Task.Started' | 'Task.Completed' | 'Task.Failed'
            task_id: maa.TaskId
            entry: string
            uuid: string
            hash: string
        }

        type TaskerContextNotify =
            | {
                  msg: 'NextList.Starting' | 'NextList.Succeeded' | 'NextList.Failed'
                  task_id: maa.TaskId
                  name: string
                  list: string[]
                  focus: unknown
              }
            | {
                  msg: 'Recognition.Starting' | 'Recognition.Succeeded' | 'Recognition.Failed'
                  task_id: maa.TaskId
                  reco_id: maa.RecoId
                  name: string
                  focus: unknown
              }
            | {
                  msg: 'Action.Starting' | 'Action.Succeeded' | 'Action.Failed'
                  task_id: maa.TaskId
                  node_id: maa.NodeId
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
                cb: (context: Context, msg: TaskerContextNotify) => MaybePromise<void>
            ): SinkId
            remove_context_sink(id: SinkId): void
            clear_context_sinks(): void
            post_task(
                entry: string,
                pipeline_override?: Record<string, unknown> | Record<string, unknown>[]
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
            node_detail(id: NodeId): NodeDetail | null
            task_detail(id: TaskId): TaskDetail | null
            latest_node(node_name: string): NodeId | null
        }
    }
}

export {}
