declare global {
    namespace maa {
        type ResourceNotify = {
            msg:
                | 'Resource.Loading.Starting'
                | 'Resource.Loading.Succeeded'
                | 'Resource.Loading.Failed'
            res_id: number // ResId
            path: string
            hash: string
        }

        /**
         * 资源类 / Resource class
         */
        class Resource {
            /**
             * 创建资源 / Create resource
             * @param handle 可选的外部句柄 / Optional external handle
             */
            constructor(handle?: string)
            /** 销毁资源 / Destroy resource */
            destroy(): void
            /**
             * 添加资源事件监听器 / Add resource event listener
             * @param cb 回调函数 / Callback function
             * @returns 监听器 id / Listener id
             */
            add_sink(cb: (res: Resource, msg: ResourceNotify) => MaybePromise<void>): SinkId
            /**
             * 移除资源事件监听器 / Remove resource event listener
             * @param id 监听器 id / Listener id
             */
            remove_sink(id: SinkId): void
            /** 清除所有资源事件监听器 / Clear all resource event listeners */
            clear_sinks(): void

            /**
             * 设置推理设备 / Set inference device
             * @param id 设备 id ('CPU' | 'Auto' | number) / Device id
             */
            set inference_device(id: 'CPU' | 'Auto' | number)
            /**
             * 设置推理执行提供者 / Set inference execution provider
             * @param provider 提供者 ('Auto' | 'CPU' | 'DirectML' | 'CoreML' | 'CUDA') / Provider
             */
            set inference_execution_provider(
                provider: 'Auto' | 'CPU' | 'DirectML' | 'CoreML' | 'CUDA',
            )

            /**
             * 注册自定义识别器 / Register a custom recognizer
             * @param name 名称 / Name
             * @param func 自定义识别器回调 / Custom recognizer callback
             */
            register_custom_recognition(name: string, func: CustomRecognitionCallback): void
            /**
             * 移除自定义识别器 / Remove the custom recognizer
             * @param name 名称 / Name
             */
            unregister_custom_recognition(name: string): void
            /** 移除所有自定义识别器 / Remove all custom recognizers */
            clear_custom_recognition(): void
            /**
             * 注册自定义操作 / Register a custom action
             * @param name 名称 / Name
             * @param func 自定义操作回调 / Custom action callback
             */
            register_custom_action(name: string, func: CustomActionCallback): void
            /**
             * 移除自定义操作 / Remove the custom action
             * @param name 名称 / Name
             */
            unregister_custom_action(name: string): void
            /** 移除所有自定义操作 / Remove all custom actions */
            clear_custom_action(): void
            /**
             * 加载资源 / Load resources from path
             * @param path 资源路径 / Resource path
             * @returns 作业对象 / Job object
             */
            post_bundle(path: string): Job<ResId, Resource>
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
             * @param next_list next 列表 / Next list
             */
            override_next(node_name: string, next_list: string[]): void
            /**
             * 覆盖图片 / Override the image corresponding to image_name
             * @param image_name 图片名 / Image name
             * @param image 图片数据 / Image data
             */
            override_image(image_name: string, image: ImageData | Buffer): void
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
             * 查询操作状态 / Query the status of operation
             * @param id 操作 id / Operation id
             * @returns 状态 / Status
             */
            status(id: ResId): Status
            /**
             * 等待操作完成 / Wait for the operation to complete
             * @param id 操作 id / Operation id
             * @returns 状态 / Status
             */
            wait(id: ResId): Promise<Status>
            /** 判断是否加载正常 / Check if resources loaded normally */
            get loaded(): boolean
            /** 获取资源 hash / Get resource hash */
            get hash(): string | null
            /** 获取任务列表 / Get task list */
            get node_list(): string[] | null
        }
    }
}

export {}
