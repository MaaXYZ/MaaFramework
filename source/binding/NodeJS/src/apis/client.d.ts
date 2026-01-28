declare global {
    namespace maa {
        class Client {
            constructor(identifier?: string)

            static create_tcp(port?: number): Client

            destroy(): void
            get identifier(): string
            bind_resource(resource: Resource): void
            register_resource_sink(resource: Resource): void
            register_controller_sink(controller: Controller): void
            register_tasker_sink(tasker: Tasker): void
            connect(): Promise<void>
            disconnect(): void
            get connected(): boolean
            get alive(): boolean
            set timeout(ms: Uint64)
            get custom_recognition_list(): string[] | null
            get custom_action_list(): string[] | null
        }
    }
}

export {}
