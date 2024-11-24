import * as maa from './maa'

export interface JobSource<Id> {
    status(id: Id): maa.Status
    wait(id: Id): Promise<maa.Status>
}

export class Job<Id, Source extends JobSource<Id>> {
    source: Source
    id: Id
    #status: maa.Status | null

    constructor(source: Source, id: Id) {
        this.source = source
        this.id = id
        this.#status = null
    }

    get status(): maa.Status {
        return this.#status ?? this.source.status(this.id)
    }

    wait() {
        const realWait = async () => {
            this.#status = await this.source.wait(this.id)
            return this
        }

        const pro = realWait() as Promise<this> & {
            status: Promise<maa.Status>
            done: Promise<boolean>
            succeeded: Promise<boolean>
            failed: Promise<boolean>
        }
        for (const key of ['status', 'done', 'succeeded', 'failed']) {
            Object.defineProperty(pro, key, {
                get: () => {
                    return new Promise(resolve => {
                        pro.then(self => {
                            resolve((self as any)[key])
                        })
                    })
                }
            })
        }
        return pro
    }

    get done() {
        return [maa.Status.Succeeded, maa.Status.Failed].includes(this.status)
    }

    get succeeded() {
        return this.status == maa.Status.Succeeded
    }

    get failed() {
        return this.status == maa.Status.Failed
    }

    get running() {
        return this.status == maa.Status.Running
    }

    get pending() {
        return this.status == maa.Status.Pending
    }
}
