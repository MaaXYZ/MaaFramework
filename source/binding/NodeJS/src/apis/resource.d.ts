declare global {
    namespace maa {
        type ResourceNotify = {
            msg: 'Loading.Starting' | 'Loading.Succeeded' | 'Loading.Failed'
            res_id: ResId
            path: string
            hash: string
        }

        class Resource {
            constructor()

            destroy(): void
            post_bundle(path: string): Job<ResId, Resource>
            override_pipeline(pipeline: Record<string, unknown> | Record<string, unknown>[]): void
            override_next(node_name: string, next_list: string[]): void
            get_node_data(node_name: string): string | null
            clear(): void
            status(id: Id): Status
            wait(id: Id): Promise<Status>
            get loaded(): boolean
            get hash(): string | null
            get node_list(): string[] | null
        }
    }
}

export {}
