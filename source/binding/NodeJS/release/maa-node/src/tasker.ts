import { ControllerBase } from './controller'
import { Job, JobSource } from './job'
import maa from './maa'
import { ResourceBase } from './resource'

type TaskDetail = ReturnType<TaskerBase['task_detail']>

type RecoDetailEntry = {
    box: maa.FlatRect
    score?: number // TemplateMatch
    count?: number // FeatureMatch | ColorMatch
    text?: string // OCR
    cls_index?: number // NeuralNetworkClassify | NeuralNetworkDetect
    label?: string // NeuralNetworkClassify | NeuralNetworkDetect
    detail?: string | Record<string, unknown> // Custom
}

export type RecoDetail = {
    all: RecoDetailEntry[]
    filtered: RecoDetailEntry[]
    best: RecoDetailEntry | null
}

class TaskJob extends Job<maa.TaskId, JobSource<maa.TaskId>> {
    #tasker: TaskerBase

    constructor(tasker: TaskerBase, source: JobSource<maa.TaskId>, id: maa.TaskId) {
        super(source, id)

        this.#tasker = tasker
    }

    get() {
        if (this.done) {
            return this.#tasker.task_detail(this.id)
        } else {
            return null
        }
    }

    wait() {
        const superPro = super.wait()
        const pro = superPro as typeof superPro & {
            get: () => Promise<TaskDetail>
        }
        pro.get = () => {
            return new Promise(resolve => {
                pro.then(self => {
                    resolve(self.get())
                })
            })
        }
        return pro
    }
}

export type TaskerNotify = {
    task_id: maa.TaskId
    entry: string
    hash: string
    uuid: string
} & (
    | {
          msg: 'Task.Started' | 'Task.Completed' | 'Task.Failed'
      }
    | {
          msg: 'NextList.Starting' | 'NextList.Succeeded' | 'NextList.Failed'
          task_id: number
          name: string
          list: string[]
          focus: unknown
      }
    | {
          msg: 'Recognition.Starting' | 'Recognition.Succeeded' | 'Recognition.Failed'
          task_id: number
          reco_id: number
          name: string
          focus: unknown
      }
    | {
          msg: 'Action.Starting' | 'Action.Succeeded' | 'Action.Failed'
          task_id: number
          node_id: number
          name: string
          focus: unknown
      }
)

export class TaskerBase {
    handle: maa.TaskerHandle
    #source: JobSource<maa.TaskId>

    notify(message: string, details_json: string): maa.MaybePromise<void> {}

    set parsed_notify(cb: (msg: TaskerNotify) => maa.MaybePromise<void>) {
        this.notify = (msg, details) => {
            return cb({
                msg: msg.replace(/^(?:Tasker|Node)?\./, '') as any,
                ...JSON.parse(details)
            })
        }
    }

    constructor(handle: maa.TaskerHandle) {
        this.handle = handle
        this.#source = {
            status: id => maa.tasker_status(this.handle, id),
            wait: id => maa.tasker_wait(this.handle, id)
        }
    }

    destroy() {
        maa.tasker_destroy(this.handle)
    }

    bind(slave: ControllerBase | ResourceBase) {
        let ret: boolean
        if (slave instanceof ControllerBase) {
            ret = maa.tasker_bind_controller(this.handle, slave.handle)
        } else {
            ret = maa.tasker_bind_resource(this.handle, slave.handle)
        }
        if (!ret) {
            throw 'Tasker bind failed'
        }
    }

    post_task(entry: string, param: Record<string, unknown> = {}) {
        return new TaskJob(
            this,
            this.#source,
            maa.tasker_post_task(this.handle, entry, JSON.stringify(param))
        )
    }

    get inited() {
        return maa.tasker_inited(this.handle)
    }

    get running() {
        return maa.tasker_running(this.handle)
    }

    post_stop() {
        return new TaskJob(this, this.#source, maa.tasker_post_stop(this.handle))
    }

    get stopping() {
        return maa.tasker_stopping(this.handle)
    }

    get resource() {
        const res = maa.tasker_get_resource(this.handle)
        if (res) {
            return new ResourceBase(res)
        } else {
            return null
        }
    }

    get controller() {
        const ctrl = maa.tasker_get_controller(this.handle)
        if (ctrl) {
            return new ControllerBase(ctrl)
        } else {
            return null
        }
    }

    clear_cache() {
        if (!maa.tasker_clear_cache(this.handle)) {
            throw 'Tasker clear_cache failed'
        }
    }

    recognition_detail(id: maa.RecoId) {
        const dt = maa.tasker_get_recognition_detail(this.handle, id)
        if (dt) {
            const [name, algorithm, hit, box, detail, raw, draws] = dt
            return {
                name,
                algorithm,
                hit,
                box,
                detail: JSON.parse(detail) as RecoDetail,
                raw,
                draws
            }
        } else {
            return null
        }
    }

    node_detail(id: maa.NodeId) {
        const dt = maa.tasker_get_node_detail(this.handle, id)
        if (dt) {
            const [name, reco_id, completed] = dt
            return {
                name,
                reco: this.recognition_detail(reco_id),
                completed
            }
        } else {
            return null
        }
    }

    task_detail(id: maa.TaskId) {
        const dt = maa.tasker_get_task_detail(this.handle, id)
        if (dt) {
            const [entry, node_ids, status] = dt
            return {
                entry,
                nodes: node_ids.map(i => this.node_detail(i)),
                status
            }
        } else {
            return null
        }
    }

    latest_node(node: string) {
        return maa.tasker_get_latest_node(this.handle, node)
    }
}

export class Tasker extends TaskerBase {
    constructor() {
        let ws: WeakRef<this>
        const h = maa.tasker_create((message, details_json) => {
            return ws.deref()?.notify(message, details_json)
        })
        if (!h) {
            throw 'Tasker create failed'
        }
        super(h)
        ws = new WeakRef(this)
    }
}
