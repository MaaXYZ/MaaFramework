declare global {
    namespace maa {
        const Global: {
            get version_from_macro(): string
            get version(): string
            set log_dir(value: string)
            set save_draw(value: boolean)
            set save_on_error(value: boolean)
            set stdout_level(
                value: 'Off' | 'Fatal' | 'Error' | 'Warn' | 'Info' | 'Debug' | 'Trace' | 'All',
            )
            set debug_mode(value: boolean)
            set draw_quality(value: number)
            set reco_image_cache_limit(value: number)
            config_init_option(user_path: string, default_json?: string): void
        }
    }
}

export {}
