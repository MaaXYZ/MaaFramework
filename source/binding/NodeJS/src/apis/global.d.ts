declare global {
    namespace maa {
        type StdoutLevel = 'Off' | 'Fatal' | 'Error' | 'Warn' | 'Info' | 'Debug' | 'Trace' | 'All'
        type MacOSPermission = 'ScreenCapture' | 'Accessibility'

        const Global: {
            get version_from_macro(): string
            get version(): string
            set log_dir(value: string)
            set save_draw(value: boolean)
            set save_on_error(value: boolean)
            set stdout_level(value: StdoutLevel)
            set debug_mode(value: boolean)
            set draw_quality(value: number)
            set reco_image_cache_limit(value: number)
            config_init_option(user_path: string, default_json?: string): void

            resize_image(image: ArrayBuffer, width: number, height: number): ArrayBuffer

            macos_check_permission(perm: MacOSPermission): boolean
            macos_request_permission(perm: MacOSPermission): Promise<boolean>
            macos_reveal_permission_settings(perm: MacOSPermission): boolean
        }
    }
}

export {}
