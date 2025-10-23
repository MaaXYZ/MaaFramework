declare global {
    const print: (msg: unknown) => void
    const readFile: (path: string) => ArrayBuffer
    const writeFile: (path: string, file: ArrayBuffer) => void
    const exit: (ret: string) => void
}

export {}
