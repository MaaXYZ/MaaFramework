declare global {
    namespace maa {
        const Global: {
            get version_from_macro(): string
            get version(): string
            set log_dir(value: string)
            set save_draw(value: boolean)
            set stdout_level(
                value: 'Off' | 'Fatal' | 'Error' | 'Warn' | 'Info' | 'Debug' | 'Trace' | 'All',
            )
            set debug_mode(value: boolean)
            config_init_option(user_path: string, default_json?: string): void

            // ProjectInterface APIs
            /** Load interface.json and read agent configuration */
            pi_load(interface_path: string): void
            /** Check if interface.json is loaded */
            get pi_loaded(): boolean
            /** Bind resource for Agent */
            pi_bind_resource(resource: maa.ResourceHandle): void
            /** Start Agent using loaded configuration */
            pi_start_agent(): void
            /** Stop Agent */
            pi_stop_agent(): void
            /** Check if Agent is running */
            get pi_agent_running(): boolean
            /** Check if Agent is connected */
            get pi_agent_connected(): boolean
        }
    }
}

export {}
