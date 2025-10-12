import maa from './maa'
import { ChainNotifyType } from './types'

export function chain_notify_impl(
    this: { notify: maa.NotificationCallback },
    cb: maa.NotificationCallback,
    order: ChainNotifyType = 'after'
) {
    const old = this.notify

    switch (order) {
        case 'before':
            this.notify = async (msg, details) => {
                await cb(msg, details)
                await old.call(this, msg, details)
            }
            break
        case 'after':
            this.notify = async (msg, details) => {
                await old.call(this, msg, details)
                await cb(msg, details)
            }
            break
        case 'before-no-wait':
            this.notify = async (msg, details) => {
                cb(msg, details)
                await old.call(this, msg, details)
            }
            break
        case 'after-no-wait':
            this.notify = async (msg, details) => {
                await old.call(this, msg, details)
                cb(msg, details)
            }
            break
        case 'replace':
            this.notify = cb
            break
    }
}
