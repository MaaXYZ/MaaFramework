declare global {
    namespace maa {
        type ResourceNotify = {
            msg:
                | 'Resource.Loading.Starting'
                | 'Resource.Loading.Succeeded'
                | 'Resource.Loading.Failed'
            res_id: ResId
            path: string
            hash: string
        }

        class Resource {
            constructor(handle?: string)
            destroy(): void
            add_sink(cb: (res: Resource, msg: ResourceNotify) => MaybePromise<void>): SinkId
            remove_sink(id: SinkId): void
            clear_sinks(): void
            post_bundle(path: string): Job<ResId, Resource>
            override_pipeline(
                pipeline_override: Record<string, unknown> | Record<string, unknown>[]
            ): void
            override_next(node_name: string, next_list: string[]): void
            get_node_data(node_name: string): string | null
            get_node_data_parsed(node_name: string): DumpTask | null
            clear(): void
            status(id: ResId): Status
            wait(id: ResId): Promise<Status>
            get loaded(): boolean
            get hash(): string | null
            get node_list(): string[] | null
        }
    }
}

export {}
