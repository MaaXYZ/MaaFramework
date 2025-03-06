import { Context } from './context'
import * as maa from './maa'
import {
    CustomActionCallback,
    CustomActionSelf,
    CustomRecognizerCallback,
    CustomRecognizerSelf
} from './types'

export const Pi = {
    __running: false,

    notify(message: string, details_json: string): maa.MaybePromise<void> {},

    register_custom_recognizer(id: maa.Id, name: string, func: CustomRecognizerCallback) {
        maa.pi_register_custom_recognizer(
            id,
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
    },

    register_custom_action(id: maa.Id, name: string, func: CustomActionCallback) {
        maa.pi_register_custom_action(id, name, (context, id, task, name, param, recoId, box) => {
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
        })
    },

    async run_cli(id: maa.Id, resource_path: string, user_path: string, directly: boolean) {
        if (Pi.__running) {
            return false
        }
        Pi.__running = true
        const res = await maa.pi_run_cli(
            id,
            resource_path,
            user_path,
            directly,
            (message, details) => {
                return Pi.notify(message, details)
            }
        )
        Pi.__running = false
        return res
    }
}
