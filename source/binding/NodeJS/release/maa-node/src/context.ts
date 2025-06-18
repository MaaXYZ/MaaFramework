import * as maa from './maa'
import { DumpTask } from './pipeline'
import { TaskerBase } from './tasker'

export class Context {
    handle: maa.ContextHandle
    #tasker: TaskerBase

    constructor(handle: maa.ContextHandle) {
        this.handle = handle
        this.#tasker = new TaskerBase(maa.context_get_tasker(this.handle))
    }

    async run_task(entry: string, pipeline_override: Record<string, unknown> = {}) {
        const id = await maa.context_run_task(this.handle, entry, JSON.stringify(pipeline_override))
        return this.#tasker.task_detail(id)
    }

    async run_recognition(
        entry: string,
        image: ArrayBuffer | Buffer,
        pipeline_override: Record<string, unknown> = {}
    ) {
        if (image instanceof Buffer) {
            image = image.buffer
        }
        const id = await maa.context_run_recognition(
            this.handle,
            entry,
            JSON.stringify(pipeline_override),
            image
        )
        return this.#tasker.recognition_detail(id)
    }

    async run_action(
        entry: string,
        box: maa.Rect,
        reco_detail: string,
        pipeline_override: Record<string, unknown> = {}
    ) {
        const id = await maa.context_run_action(
            this.handle,
            entry,
            JSON.stringify(pipeline_override),
            box,
            reco_detail
        )
        return this.#tasker.node_detail(id)
    }

    override_pipeline(pipeline_override: Record<string, unknown>) {
        if (!maa.context_override_pipeline(this.handle, JSON.stringify(pipeline_override))) {
            throw 'Context override_pipeline failed'
        }
    }

    override_next(node_name: string, next: string[]) {
        if (!maa.context_override_next(this.handle, node_name, next)) {
            throw 'Context override_next failed'
        }
    }

    get_node_data(node_name: string) {
        return maa.context_get_node_data(this.handle, node_name)
    }

    get_node_data_parsed(node_name: string) {
        const content = this.get_node_data(node_name)
        if (content) {
            return JSON.parse(content) as DumpTask
        } else {
            return null
        }
    }

    get task_id() {
        return maa.context_get_task_id(this.handle)
    }

    get tasker() {
        return this.#tasker
    }

    clone() {
        return new Context(maa.context_clone(this.handle))
    }
}
