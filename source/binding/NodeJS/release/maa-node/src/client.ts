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

    create_socket(identifier: string | null) {
        const rid = maa.agent_client_create_socket(this.handle, identifier)
        if (rid === null) {
            throw 'AgentClient create socket failed'
        }
        return rid
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
}
