declare global {
    namespace maa {
        type ResourceLoadError = {
            code: string
            file?: string
            node?: string
            pointer?: string
            field?: string
            target?: string
        }
        type ResourceLoadDetail = {
            res_id: number // ResId
            path: string
            type: 'Bundle' | 'OcrModel' | 'Pipeline' | 'Image'
            hash: string
            status: Status
            errors: ResourceLoadError[]
        }
        type ResourceNotify = {
            msg: NotifyMessage<'Loading'>
            status?: Status
            errors?: ResourceLoadError[]
            res_id: number // ResId
            path: string
            hash: string
        }

        class Resource {
            constructor(handle?: string)
            destroy(): void
            add_sink(cb: (res: Resource, msg: ResourceNotify) => MaybePromise<void>): SinkId
            remove_sink(id: SinkId): void
            clear_sinks(): void

            set inference_device(id: 'CPU' | 'Auto' | number)
            /**
             * `CoreML` is deprecated. MaaDeps no longer ships it; use `WebGPU`.
             * `NCNN` is Android only (CPU), setting it on other platforms throws.
             * Switching to or from `NCNN` after resource loading starts throws until the resource is cleared.
             */
            set inference_execution_provider(
                provider: 'Auto' | 'CPU' | 'DirectML' | 'WebGPU' | 'CoreML' | 'CUDA' | 'NCNN',
            )

            /**
             * Configure a local Resource before post_*; omitted config enables discovery.
             * clear() resets it. Returns false when unsupported or invalid.
             */
            configure_wechat_ocr(config?: WeChatOCRConfig | string): boolean

            register_custom_recognition(name: string, func: CustomRecognitionCallback): void
            unregister_custom_recognition(name: string): void
            clear_custom_recognition(): void
            register_custom_action(name: string, func: CustomActionCallback): void
            unregister_custom_action(name: string): void
            clear_custom_action(): void
            post_bundle(path: string): Job<ResId, Resource>
            post_ocr_model(path: string): Job<ResId, Resource>
            post_pipeline(path: string): Job<ResId, Resource>
            post_image(path: string): Job<ResId, Resource>
            override_pipeline(
                pipeline_override: Record<string, unknown> | Record<string, unknown>[],
            ): void
            override_next(node_name: string, next_list: string[]): void
            override_image(image_name: string, image: ImageData): void
            get_node_data(node_name: string): string | null
            get_node_data_parsed(node_name: string): DumpTask | null
            get_default_recognition_param(
                reco_type: RecognitionType,
            ): Exclude<Recognition<ModeFragment>['recognition'], string | undefined>['param'] | null
            get_default_action_param(
                action_type: ActionType,
            ): Exclude<Action<ModeFragment>['action'], string | undefined>['param'] | null
            clear(): void
            status(id: ResId): Status
            wait(id: ResId): Promise<Status>
            get loaded(): boolean
            get_load_detail(id: ResId): ResourceLoadDetail | null
            get hash(): string | null
            get node_list(): string[] | null
            get custom_recognition_list(): string[] | null
            get custom_action_list(): string[] | null
        }

        type WeChatOCRConfig = {
            sdk_path?: string
            runtime_dir?: string
            ocr_path?: string
            worker_path?: string
            temp_dir?: string
            protocol?: 0 | 3 | 4
            startup_timeout_ms?: number
            request_timeout_ms?: number
            max_pending_requests?: number
            search_roots?: string[]
            scan_system?: boolean
        }
    }
}

export {}
