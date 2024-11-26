import { Context } from './context'
import { Job, JobSource } from './job'
import maa from './maa'
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

    set inference_device(id: 'cpu' | 'auto' | number) {
        switch (id) {
            case 'cpu':
                id = -2
                break
            case 'auto':
                id = -1
                break
        }
        if (!maa.resource_set_option_inference_device(this.handle, id)) {
            throw 'Resource set inference_device failed'
        }
    }

    register_custom_recognizer(name: string, func: CustomRecognizerCallback) {
        if (
            !maa.resource_register_custom_recognition(
                this.handle,
                name,
                (context, id, task, name, param, image) => {
                    const self: CustomRecognizerSelf = {
                        context: new Context(context),
                        id,
                        task,
                        name,
                        param: JSON.parse(param),
                        image
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

    post_path(path: string) {
        return new Job(this.#source, maa.resource_post_path(this.handle, path))
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
        return maa.resource_get_task_list(this.handle)
    }
}

export class Resource extends ResourceBase {
    constructor() {
        let ws: WeakRef<this>
        const h = maa.resource_create((message, details_json) => {
            ws.deref()?.notify(message, details_json)
        })
        if (!h) {
            throw 'Resource create failed'
        }
        super(h)
        ws = new WeakRef(this)
    }
}
