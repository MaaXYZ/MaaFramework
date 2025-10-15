declare global {
    namespace maa {
        const Global: {
            get version_from_macro(): string
            get version(): string
        }
    }
}

export {}
