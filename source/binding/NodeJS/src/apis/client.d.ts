declare global {
    namespace maa {
        class Client {
            constructor(identifier?: string)

            destroy(): void
            get identifier(): string
            bind_resource(resource: Resource): void
            register_resource_sink(resource: Resource): void
            register_controller_sink(controller: Controller): void
            register_tasker_sink(tasker: Tasker): void
            connect(): Promise<boolean>
            disconnect(): void
            get connected(): boolean
            get alive(): boolean
            set timeout(ms: Uint64)
            get custom_recognition_list(): string[] | null
            get custom_action_list(): string[] | null
            get custom_controller_list(): string[] | null
            /** Keep this client alive and connected while using the controller. Create a new controller after reconnecting. */
            create_custom_controller(name: string): Controller
        }

        class PluginController extends Controller {
            constructor(name: string, config_json?: string)
            static load(path: string): boolean
            static get names(): string[] | null
        }
    }
}

export {}
