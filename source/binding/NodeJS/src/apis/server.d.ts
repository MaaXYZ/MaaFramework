declare global {
    namespace maa {
        interface AgentControllerActor extends CustomControllerActor {
            connected?(): MaybePromise<boolean>
        }

        const Server: {
            register_custom_recognition(name: string, func: CustomRecognitionCallback): void
            register_custom_action(name: string, func: CustomActionCallback): void
            /**
             * Register before start_up. Proxies for the same name share this actor.
             * Keep this JavaScript environment alive until the server stops (normally await Server.join()).
             */
            register_custom_controller(name: string, actor: AgentControllerActor): void

            add_resource_sink(cb: (res: Resource, msg: ResourceNotify) => MaybePromise<void>): void
            add_controller_sink(
                cb: (ctrl: Controller, msg: ControllerNotify) => MaybePromise<void>,
            ): void
            add_tasker_sink(cb: (tsk: Tasker, msg: TaskerNotify) => MaybePromise<void>): void
            add_context_sink(
                cb: (ctx: Context, msg: TaskerContextNotify) => MaybePromise<void>,
            ): void

            start_up(identifier: string): Promise<boolean>
            shut_down(): Promise<void>
            join(): Promise<void>
            detach(): void
        }
    }
}

export {}
