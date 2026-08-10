declare global {
    namespace maa {
        class PortalHelper {
            destroy(): void
            open_stream(): Promise<boolean>

            set persist(value: boolean)
            get pipewire_fd(): number
            get pipewire_node_id(): number
            get restore_token(): string | null
            set restore_token(value: string)
        }
    }
}

export {}
