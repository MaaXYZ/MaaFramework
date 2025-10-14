declare module '@nekosu/maa' {
    type Uint64 = string
    type Id = Uint64
    type ScreencapOrInputMethods = Uint64

    type ResId = Id & { __brand: 'ResId' }
    type CtrlId = Id & { __brand: 'CtrlId' }
    type TaskId = Id & { __brand: 'TaskId' }
    type RecoId = Id & { __brand: 'RecoId' }
    type NodeId = Id & { __brand: 'NodeId' }

    type Status = number & { __brand: 'Status' }
    type LoggingLevel = number & { __brand: 'LoggingLevel' }
    type InferenceDevice = number & { __brand: 'InferenceDevice' }
    type InferenceExecutionProvider = number & { __brand: 'InferenceExecutionProvider' }

    type ImageData = ArrayBuffer

    // global.cpp

    const Global: {
        get version_from_macro(): string
        get version(): string
    }

    // job.cpp

    interface JobSource<Id> {
        status(id: Id): Status
        wait(id: Id): Promise<Status>
    }

    class Job<Id, Source extends JobSource<Id>> {
        constructor(source: Source, id: Id)

        get source(): Source
        get id(): Id
        get status(): Status
        wait(): Promise<this> & {
            status: Promise<Status>
            done: Promise<boolean>
            succeeded: Promise<boolean>
            failed: Promise<boolean>
        }
        get done(): boolean
        get succeeded(): boolean
        get failed(): boolean
        get running(): boolean
        get pending(): boolean
    }

    // resource.cpp

    type ResourceNotify = {
        msg: 'Loading.Starting' | 'Loading.Succeeded' | 'Loading.Failed'
        res_id: ResId
        path: string
        hash: string
    }

    class Resource {
        status(id: Id): Status
        wait(id: Id): Promise<Status>
    }
}
