import { Context } from './context'
import { Job, JobSource } from './job'
import maa from './maa'
import { DumpTask } from './pipeline'
import {
    CustomActionCallback,
    CustomActionSelf,
    CustomRecognizerCallback,
    CustomRecognizerSelf
} from './types'

export type ResourceNotify = { res_id: maa.ResId; path: string } & (
    | {
          msg: 'StartLoading'
      }
    | {
          msg: 'LoadingCompleted' | 'LoadingFailed'
          hash: string
      }
)

export class ResourceBase {
    handle: maa.ResourceHandle
    #source: JobSource<maa.ResId>

    notify(message: string, details_json: string): maa.MaybePromise<void> {}

    set parsed_notify(cb: (msg: ResourceNotify) => maa.MaybePromise<void>) {
        this.notify = (msg, details) => {
            return cb({
                msg: msg.replace(/^Resource\./, '') as any,
                ...JSON.parse(details)
            })
        }
    }

    constructor(handle: maa.ResourceHandle) {
        this.handle = handle
        this.#source = {
            status: id => maa.resource_status(this.handle, id),
            wait: id => maa.resource_wait(this.handle, id)
        }
    }

    destroy() {
        maa.resource_destroy(this.handle)
    }

    set inference_device(id: keyof typeof maa.InferenceDevice | number) {
        if (typeof id === 'string') {
            id = maa.InferenceDevice[id]
        }
        if (!maa.resource_set_option_inference_device(this.handle, id)) {
            throw 'Resource set inference_device failed'
        }
    }

    set inference_execution_provider(provider: keyof typeof maa.InferenceExecutionProvider) {
        if (
            !maa.resource_set_option_inference_execution_provider(
                this.handle,
                maa.InferenceExecutionProvider[provider]
            )
        ) {
            throw 'Resource set inference_execution_provider failed'
        }
    }

    register_custom_recognizer(name: string, func: CustomRecognizerCallback) {
        if (
            !maa.resource_register_custom_recognition(
                this.handle,
                name,
                (context, id, task, name, param, image, roi) => {
                    const self: CustomRecognizerSelf = {
                        context: new Context(context),
                        id,
                        task,
                        name,
                        param: JSON.parse(param),
                        image,
                        roi
                    }
                    return func.apply(self, [self])
                }
            )
        ) {
            throw 'Resource register_custom_recognition failed'
        }
    }

    unregister_custom_recognizer(name: string) {
        if (!maa.resource_unregister_custom_recognition(this.handle, name)) {
            throw 'Resource unregister_custom_recognition failed'
        }
    }

    clear_custom_recognizer() {
        if (!maa.resource_clear_custom_recognition(this.handle)) {
            throw 'Resource clear_custom_recognition failed'
        }
    }

    register_custom_action(name: string, func: CustomActionCallback) {
        if (
            !maa.resource_register_custom_action(
                this.handle,
                name,
                (context, id, task, name, param, recoId, box) => {
                    const self: CustomActionSelf = {
                        context: new Context(context),
                        id,
                        task,
                        name,
                        param: JSON.parse(param),
                        recoId,
                        box
                    }
                    return func.apply(self, [self])
                }
            )
        ) {
            throw 'Resource register_custom_action failed'
        }
    }

    unregister_custom_action(name: string) {
        if (!maa.resource_unregister_custom_action(this.handle, name)) {
            throw 'Resource unregister_custom_action failed'
        }
    }

    clear_custom_action() {
        if (!maa.resource_clear_custom_action(this.handle)) {
            throw 'Resource clear_custom_action failed'
        }
    }

    post_bundle(path: string) {
        return new Job(this.#source, maa.resource_post_bundle(this.handle, path))
    }

    override_pipeline(pipeline_override: string) {
        if (!maa.resource_override_pipeline(this.handle, pipeline_override)) {
            throw 'Resource override_pipeline failed'
        }
    }

    override_next(node_name: string, next_list: string[]) {
        if (!maa.resource_override_next(this.handle, node_name, next_list)) {
            throw 'Resource override_next failed'
        }
    }

    get_node_data(node_name: string) {
        return maa.resource_get_node_data(this.handle, node_name)
    }

    get_node_data_parsed(node_name: string) {
        const content = this.get_node_data(node_name)
        if (content) {
            return JSON.parse(content) as DumpTask
        } else {
            return null
        }
    }

    clear() {
        if (!maa.resource_clear(this.handle)) {
            throw 'Resource clear failed'
        }
    }

    get loaded() {
        return maa.resource_loaded(this.handle)
    }

    get hash() {
        return maa.resource_get_hash(this.handle)
    }

    get task_list() {
        return maa.resource_get_node_list(this.handle)
    }
}

export class Resource extends ResourceBase {
    constructor() {
        let ws: WeakRef<this>
        const h = maa.resource_create((message, details_json) => {
            return ws.deref()?.notify(message, details_json)
        })
        if (!h) {
            throw 'Resource create failed'
        }
        super(h)
        ws = new WeakRef(this)
    }
}
