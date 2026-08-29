declare global {
    namespace maa {
        const Server: {
            register_custom_recognition(name: string, func: CustomRecognitionCallback): void
            register_custom_action(name: string, func: CustomActionCallback): void

            add_resource_sink(cb: (res: Resource, msg: ResourceNotify) => MaybePromise<void>): void
            add_controller_sink(
                cb: (ctrl: Controller, msg: ControllerNotify) => MaybePromise<void>,
            ): void
            add_tasker_sink(cb: (tsk: Tasker, msg: TaskerNotify) => MaybePromise<void>): void
            add_context_sink(
                cb: (ctx: Context, msg: TaskerContextNotify) => MaybePromise<void>,
            ): void

            /**
             * Invoked when the AgentServer receives a ShutDownRequest, before the
             * message loop stops; the ShutDownResponse is replied only after the
             * callback (including the returned Promise) settles. Must be set
             * before start_up; re-registration overwrites.
             * Do NOT call maa.Server.join / maa.Server.shut_down inside the callback,
             * and do NOT throw synchronously (it terminates the process); a rejected
             * Promise is silently ignored and shutdown continues.
             */
            set_shutdown_callback(cb: () => MaybePromise<void>): void

            start_up(identifier: string): Promise<boolean>
            shut_down(): Promise<void>
            join(): Promise<void>
            detach(): void
        }
    }
}

export {}
