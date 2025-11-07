declare global {
    namespace maa {
        /**
         * Agent 服务端对象 / Agent server object
         */
        const Server: {
            /**
             * 注册自定义识别器 / Register a custom recognizer
             * @param name 名称 / Name
             * @param func 自定义识别器回调 / Custom recognizer callback
             */
            register_custom_recognition(name: string, func: CustomRecognitionCallback): void
            /**
             * 注册自定义操作 / Register a custom action
             * @param name 名称 / Name
             * @param func 自定义操作回调 / Custom action callback
             */
            register_custom_action(name: string, func: CustomActionCallback): void

            /**
             * 添加资源事件监听器 / Add resource event listener
             * @param cb 回调函数 / Callback function
             * @returns 监听器 id / Listener id
             */
            add_resource_sink(cb: (res: Resource, msg: ResourceNotify) => MaybePromise<void>): void
            /**
             * 添加控制器事件监听器 / Add controller event listener
             * @param cb 回调函数 / Callback function
             * @returns 监听器 id / Listener id
             */
            add_controller_sink(
                cb: (ctrl: Controller, msg: ControllerNotify) => MaybePromise<void>,
            ): void
            /**
             * 添加实例事件监听器 / Add instance event listener
             * @param cb 回调函数 / Callback function
             * @returns 监听器 id / Listener id
             */
            add_tasker_sink(cb: (tsk: Tasker, msg: TaskerNotify) => MaybePromise<void>): void
            /**
             * 添加上下文事件监听器 / Add context event listener
             * @param cb 回调函数 / Callback function
             * @returns 监听器 id / Listener id
             */
            add_context_sink(
                cb: (ctx: Context, msg: TaskerContextNotify) => MaybePromise<void>,
            ): void

            /**
             * 启动服务并连接到指定地址 / Start server and connect to identifier
             * @param identifier 连接地址 / Connection address
             * @returns 是否成功 / Whether successful
             */
            start_up(identifier: string): Promise<boolean>
            /** 停止服务 / Stop server */
            shut_down(): Promise<void>
            /** 同步等待服务线程结束 / Synchronously wait for server thread to end */
            join(): Promise<void>
            /** 分离服务线程 / Detach server thread */
            detach(): void
        }
    }
}

export {}
