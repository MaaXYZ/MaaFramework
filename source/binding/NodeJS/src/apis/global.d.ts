declare global {
    namespace maa {
        /**
         * 全局配置对象 / Global configuration object
         */
        const Global: {
            /** 获取版本（从宏定义） / Get version (from macro) */
            get version_from_macro(): string
            /** 获取版本 / Get version */
            get version(): string
            /**
             * 设置日志路径 / Set the log path
             * @param value 日志路径 / Log path
             */
            set log_dir(value: string)
            /**
             * 设置是否将识别保存到日志路径/vision中 / Set whether to save recognition results to log path/vision
             * 
             * 开启后 RecoDetail 将可以获取到 draws
             * When enabled, RecoDetail can retrieve draws
             * 
             * @param value 是否保存 / Whether to save
             */
            set save_draw(value: boolean)
            /**
             * 设置日志输出到 stdout 中的级别 / Set the log output level to stdout
             * @param value 日志级别 / Logging level
             */
            set stdout_level(
                value: 'Off' | 'Fatal' | 'Error' | 'Warn' | 'Info' | 'Debug' | 'Trace' | 'All',
            )
            /**
             * 设置是否启用调试模式 / Set whether to enable debug mode
             * 
             * 调试模式下, RecoDetail 将可以获取到 raw/draws; 所有任务都会被视为 focus 而产生回调
             * In debug mode, RecoDetail can retrieve raw/draws; all tasks are treated as focus and produce callbacks
             * 
             * @param value 是否启用调试模式 / Whether to enable debug mode
             */
            set debug_mode(value: boolean)
            /**
             * 从 user_path 中加载全局配置 / Load global config from user_path
             * @param user_path 配置存储路径 / Config storage path
             * @param default_json 默认配置 / Default config
             */
            config_init_option(user_path: string, default_json?: string): void
        }
    }
}

export {}
