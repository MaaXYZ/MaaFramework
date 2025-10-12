import { ControllerBase } from './controller'
import * as maa from './maa'
import { Resource } from './resource'
import { Tasker } from './tasker'

export class AgentClient {
    handle: maa.AgentClientHandle

    constructor(identifier?: string) {
        const h = maa.agent_client_create(identifier ?? null)
        if (!h) {
            throw 'AgentClient create failed'
        }
        this.handle = h
    }

    destroy() {
        maa.agent_client_destroy(this.handle)
    }

    get identifier() {
        return maa.agent_client_identifier(this.handle)
    }

    bind_resource(resource: Resource) {
        if (!maa.agent_client_bind_resource(this.handle, resource.handle)) {
            throw 'AgentClient bind resource failed'
        }
    }

    register_sink(tasker?: Tasker, resource?: Resource, controller?: ControllerBase) {
        if (
            !maa.agent_client_register_sink(
                this.handle,
                tasker?.handle ?? null,
                resource?.handle ?? null,
                controller?.handle ?? null
            )
        ) {
            throw 'AgentClient register sink failed'
        }
    }

    async connect() {
        if (!(await maa.agent_client_connect(this.handle))) {
            throw 'AgentClient connect failed'
        }
    }

    disconnect() {
        if (!maa.agent_client_disconnect(this.handle)) {
            throw 'AgentClient disconnect failed'
        }
    }

    get connected() {
        return maa.agent_client_connected(this.handle)
    }

    get alive() {
        return maa.agent_client_alive(this.handle)
    }

    set timeout(ms: maa.Uint64) {
        if (!maa.agent_client_set_timeout(this.handle, ms)) {
            throw 'AgentClient set timeout failed'
        }
    }
}
