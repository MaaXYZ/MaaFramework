declare global {
    namespace maa {
        class Client {
            constructor(identifier?: string)

            destroy(): void
            get identifier(): string
            bind_resource(resource: Resource): void
            register_sink(tasker?: Tasker, resource?: Resource, controller?: Controller): void
            connect(): Promise<void>
            disconnect(): void
            get connected(): boolean
            get alive(): boolean
            set timeout(ms: Uint64)
        }
    }
}

export {}
