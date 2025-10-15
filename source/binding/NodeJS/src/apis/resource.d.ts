declare module '@nekosu/maa' {
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
        status(id: Id): Status
        wait(id: Id): Promise<Status>
    }
}
