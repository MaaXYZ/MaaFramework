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

    start_child(identifier: string | null) {
        const rid = maa.agent_client_start_child(this.handle, identifier)
        if (rid === null) {
            throw 'AgentClient start child failed'
        }
        return rid
    }

    connect() {
        if (!maa.agent_client_connect(this.handle)) {
            throw 'AgentClient connect failed'
        }
    }
}
