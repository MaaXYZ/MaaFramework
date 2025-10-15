declare module '@nekosu/maa' {
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
}
