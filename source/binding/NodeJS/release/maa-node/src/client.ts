import * as maa from './maa'
import { Resource } from './resource'

export class AgentClient {
    handle: maa.AgentClientHandle

    constructor() {
        const h = maa.agent_client_create()
        if (!h) {
            throw 'AgentClient create failed'
        }
        this.handle = h
    }

    destroy() {
        maa.agent_client_destroy(this.handle)
    }

    bind_resource(resource: Resource) {
        if (!maa.agent_client_bind_resource(this.handle, resource.handle)) {
            throw 'AgentClient bind resource failed'
        }
    }

    start_child(exec: string, args: string[]) {
        if (!maa.agent_client_start_child(this.handle, exec, args)) {
            throw 'AgentClient start child failed'
        }
    }
}
