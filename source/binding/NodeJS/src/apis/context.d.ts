declare global {
    namespace maa {
        /**
         * 上下文类 / Context class
         * 
         * 上下文是运行任务、识别、操作和其他操作的上下文
         * Context is a context for running tasks, recognitions, actions, and other operations
         */
        class Context {
            /**
             * 创建上下文 / Create context
             * @param handle 可选的外部句柄 / Optional external handle
             */
            constructor(handle?: string)

            /**
             * 同步执行任务 / Synchronously execute task
             * @param entry 任务入口 / Task entry
             * @param pipeline_override 用于覆盖的 json / JSON for overriding
             * @returns 任务详情，如果不存在则返回 null / Task detail, or null if not exists
             */
            run_task(
                entry: string,
                pipeline_override?: Record<string, unknown> | Record<string, unknown>[],
            ): Promise<TaskDetail | null>
            /**
             * 同步执行识别逻辑 / Synchronously execute recognition logic
             * 
             * 不会执行后续操作, 不会执行后续 next
             * Will not execute subsequent operations or next steps
             * 
             * @param entry 任务名 / Task name
             * @param image 前序截图 / Previous screenshot
             * @param pipeline_override 用于覆盖的 json / JSON for overriding
             * @returns 识别详情，如果不存在则返回 null / Recognition detail, or null if not exists
             */
            run_recognition(
                entry: string,
                image: ImageData | Buffer,
                pipeline_override?: Record<string, unknown> | Record<string, unknown>[],
            ): Promise<RecoDetail | null>
            /**
             * 同步执行操作逻辑 / Synchronously execute action logic
             * 
             * 不会执行后续 next
             * Will not execute subsequent next steps
             * 
             * @param entry 任务名 / Task name
             * @param box 前序识别位置 / Previous recognition position
             * @param reco_detail 前序识别详情 / Previous recognition details
             * @param pipeline_override 用于覆盖的 json / JSON for overriding
             * @returns 节点详情，如果不存在则返回 null / Node detail, or null if not exists
             */
            run_action(
                entry: string,
                box: Rect,
                reco_detail: string,
                pipeline_override?: Record<string, unknown> | Record<string, unknown>[],
            ): Promise<NodeDetail | null>
            /**
             * 覆盖 pipeline / Override pipeline_override
             * @param pipeline_override 用于覆盖的 json / JSON for overriding
             */
            override_pipeline(
                pipeline_override: Record<string, unknown> | Record<string, unknown>[],
            ): void
            /**
             * 覆盖任务的 next 列表 / Override the next list of task
             * @param node_name 任务名 / Task name
             * @param next next 列表 / Next list
             */
            override_next(node_name: string, next: string[]): void
            /**
             * 覆盖图片 / Override the image corresponding to image_name
             * @param image_name 图片名 / Image name
             * @param image 图片数据 / Image data
             */
            override_image(image_name: string, image: ImageData | Buffer): void
            /**
             * 获取任务当前的定义 / Get current definition of task
             * @param node_name 任务名 / Task name
             * @returns 任务定义字符串，如果不存在则返回 null / Task definition string, or null if not exists
             */
            get_node_data(node_name: string): string | null
            /**
             * 获取任务当前的定义（已解析） / Get current definition of task (parsed)
             * @param node_name 任务名 / Task name
             * @returns 任务定义对象，如果不存在则返回 null / Task definition object, or null if not exists
             */
            get_node_data_parsed(node_name: string): DumpTask | null
            /** 获取对应任务号 / Get corresponding task id */
            get task_id(): TaskId
            /** 获取实例 / Get instance */
            get tasker(): Tasker
            /**
             * 复制上下文 / Clone context
             * @returns 复制的上下文对象 / Cloned context object
             */
            clone(): Context
        }
    }
}

export {}
