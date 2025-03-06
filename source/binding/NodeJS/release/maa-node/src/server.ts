import { Context } from './context'
import * as maa from './maa'
import {
    CustomActionCallback,
    CustomActionSelf,
    CustomRecognizerCallback,
    CustomRecognizerSelf
} from './types'

export const AgentServer = {
    register_custom_recognizer(name: string, func: CustomRecognizerCallback) {
        maa.agent_server_register_custom_recognition(
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

    register_custom_action(name: string, func: CustomActionCallback) {
        maa.agent_server_register_custom_action(
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
    },

    start_up(args: string[]) {
        if (!maa.agent_server_start_up(args)) {
            throw 'AgentServer start up failed'
        }
    },

    shut_down() {
        maa.agent_server_shut_down()
    }
}
