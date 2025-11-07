declare global {
    namespace maa {
        const Plugin: {
            add_resource_sink(cb: (res: Resource, msg: ResourceNotify) => MaybePromise<void>): void
            add_controller_sink(
                cb: (ctrl: Controller, msg: ControllerNotify) => MaybePromise<void>,
            ): void
            add_tasker_sink(cb: (tsk: Tasker, msg: TaskerNotify) => MaybePromise<void>): void
            add_context_sink(
                cb: (ctx: Context, msg: TaskerContextNotify) => MaybePromise<void>,
            ): void
        }
    }
}

export {}
