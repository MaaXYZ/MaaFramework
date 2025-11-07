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
                  name: string
                  focus: unknown
              }

        /** 任务作业类 / Task job class */
        class TaskJob extends Job<TaskId, Tasker, TaskDetail> {}

        /**
         * 实例类 / Instance class
         */
        class Tasker {
            /**
             * 创建实例 / Create instance
             * @param handle 可选的外部句柄 / Optional external handle
             */
            constructor(handle?: string)
            /** 销毁实例 / Destroy instance */
            destroy(): void
            /**
             * 添加实例事件监听器 / Add instance event listener
             * @param cb 回调函数 / Callback function
             * @returns 监听器 id / Listener id
             */
            add_sink(cb: (tasker: Tasker, msg: TaskerNotify) => MaybePromise<void>): SinkId
            /**
             * 移除实例事件监听器 / Remove instance event listener
             * @param id 监听器 id / Listener id
             */
            remove_sink(id: SinkId): void
            /** 清除所有实例事件监听器 / Clear all instance event listeners */
            clear_sinks(): void
            /**
             * 添加上下文事件监听器 / Add context event listener
             * @param cb 回调函数 / Callback function
             * @returns 监听器 id / Listener id
             */
            add_context_sink(
                cb: (context: Context, msg: TaskerContextNotify) => MaybePromise<void>,
            ): SinkId
            /**
             * 移除上下文事件监听器 / Remove context event listener
             * @param id 监听器 id / Listener id
             */
            remove_context_sink(id: SinkId): void
            /** 清除所有上下文事件监听器 / Clear all context event listeners */
            clear_context_sinks(): void
            /**
             * 执行任务 / Execute task
             * @param entry 任务入口 / Task entry
             * @param pipeline_override 用于覆盖的 json / JSON for overriding
             * @returns 任务作业对象 / Task job object
             */
            post_task(
                entry: string,
                pipeline_override?: Record<string, unknown> | Record<string, unknown>[],
            ): TaskJob
            /**
             * 停止实例 / Stop instance
             * @returns 作业对象 / Job object
             */
            post_stop(): TaskJob
            /**
             * 查询操作状态 / Query status of operation
             * @param id 操作 id / Operation id
             * @returns 状态 / Status
             */
            status(id: TaskId): Status
            /**
             * 等待操作完成 / Wait for operation to complete
             * @param id 操作 id / Operation id
             * @returns 状态 / Status
             */
            wait(id: TaskId): Promise<Status>
            /** 判断是否正确初始化 / Check if initialized correctly */
            get inited(): boolean
            /** 判断实例是否还在运行 / Check if instance is still running */
            get running(): boolean
            /** 判断实例是否正在停止中(尚未停止) / Check if instance is stopping (not yet stopped) */
            get stopping(): boolean
            /**
             * 设置关联的资源 / Set bound resource
             * @param res 资源对象 / Resource object
             */
            set resource(res: Resource | null)
            /** 获取关联的资源 / Get bound resource */
            get resource(): Resource | null
            /**
             * 设置关联的控制器 / Set bound controller
             * @param res 控制器对象 / Controller object
             */
            set controller(res: Controller | null)
            /** 获取关联的控制器 / Get bound controller */
            get controller(): Controller | null
            /** 清理所有可查询的信息 / Clear all queryable information */
            clear_cache(): void
            /**
             * 获取识别信息 / Get recognition info
             * @param id 识别号 / Recognition id
             * @returns 识别详情，如果不存在则返回 null / Recognition detail, or null if not exists
             */
            recognition_detail(id: RecoId): RecoDetail | null
            /**
             * 获取节点信息 / Get node info
             * @param id 节点号 / Node id
             * @returns 节点详情，如果不存在则返回 null / Node detail, or null if not exists
             */
            node_detail(id: NodeId): NodeDetail | null
            /**
             * 获取任务信息 / Get task info
             * @param id 任务号 / Task id
             * @returns 任务详情，如果不存在则返回 null / Task detail, or null if not exists
             */
            task_detail(id: TaskId): TaskDetail | null
            /**
             * 获取任务的最新节点号 / Get latest node id for task
             * @param node_name 任务名 / Task name
             * @returns 节点号，如果不存在则返回 null / Node id, or null if not exists
             */
            latest_node(node_name: string): NodeId | null
        }
    }
}

export {}
