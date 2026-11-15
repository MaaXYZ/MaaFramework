declare global {
    namespace maa {
        type PluginLifecycleNotify = { msg: 'Created' | 'Destroying' }
        const Plugin: {
            add_resource_sink(cb: (res: Resource, msg: ResourceNotify | PluginLifecycleNotify) => MaybePromise<void>): void
            add_controller_sink(
                cb: (ctrl: Controller, msg: ControllerNotify | PluginLifecycleNotify) => MaybePromise<void>,
            ): void
            add_tasker_sink(cb: (tsk: Tasker, msg: TaskerNotify | PluginLifecycleNotify) => MaybePromise<void>): void
            add_context_sink(
                cb: (ctx: Context, msg: TaskerContextNotify) => MaybePromise<void>,
            ): void
        }
    }
}

export {}
