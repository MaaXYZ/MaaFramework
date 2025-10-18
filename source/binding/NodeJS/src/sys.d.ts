declare global {
    const print: (msg: string) => void
    const save: (path: string, file: ArrayBuffer) => void
    const exit: (ret: string) => void
}

export {}
