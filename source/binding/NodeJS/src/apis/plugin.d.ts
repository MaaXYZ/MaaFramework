declare global {
    namespace maa {
        /**
         * 插件对象 / Plugin object
         * 
         * 用于在插件中注册事件监听器
         * Used to register event listeners in plugins
         */
        const Plugin: {
            /**
             * 添加资源事件监听器 / Add resource event listener
             * @param cb 回调函数 / Callback function
             */
            add_resource_sink(cb: (res: Resource, msg: ResourceNotify) => MaybePromise<void>): void
            /**
             * 添加控制器事件监听器 / Add controller event listener
             * @param cb 回调函数 / Callback function
             */
            add_controller_sink(
                cb: (ctrl: Controller, msg: ControllerNotify) => MaybePromise<void>,
            ): void
            /**
             * 添加实例事件监听器 / Add instance event listener
             * @param cb 回调函数 / Callback function
             */
            add_tasker_sink(cb: (tsk: Tasker, msg: TaskerNotify) => MaybePromise<void>): void
            /**
             * 添加上下文事件监听器 / Add context event listener
             * @param cb 回调函数 / Callback function
             */
            add_context_sink(
                cb: (ctx: Context, msg: TaskerContextNotify) => MaybePromise<void>,
            ): void
        }
    }
}

export {}
