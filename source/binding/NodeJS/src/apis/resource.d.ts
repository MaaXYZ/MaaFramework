declare global {
    namespace maa {
        type ResourceNotify = {
            msg: NotifyMessage<'Loading'>
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
            set inference_execution_provider(
                provider: 'Auto' | 'CPU' | 'DirectML' | 'CoreML' | 'CUDA',
            )

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
            clear(): void
            status(id: ResId): Status
            wait(id: ResId): Promise<Status>
            get loaded(): boolean
            get hash(): string | null
            get node_list(): string[] | null
            get custom_recognition_list(): string[] | null
            get custom_action_list(): string[] | null
        }
    }
}

export {}
