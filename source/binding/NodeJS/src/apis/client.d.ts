declare global {
    namespace maa {
        /**
         * Agent 客户端类 / Agent client class
         */
        class Client {
            /**
             * 创建 Agent 客户端 / Create Agent client
             * @param identifier 监听地址，为空则自动生成 / Listening address, auto-generate if empty
             */
            constructor(identifier?: string)

            /** 销毁 Agent 客户端 / Destroy Agent client */
            destroy(): void
            /** 获取实际监听地址 / Get actual listening address */
            get identifier(): string
            /**
             * 将 Agent 客户端关联到资源 / Bind Agent client to resource
             * @param resource 资源对象 / Resource object
             */
            bind_resource(resource: Resource): void
            /** 连接 / Connect */
            connect(): Promise<void>
            /** 断开连接 / Disconnect */
            disconnect(): void
            /** 判断是否已连接 / Check if connected */
            get connected(): boolean
            /** 判断 Agent 服务端是否仍可响应 / Check if Agent server is still responsive */
            get alive(): boolean
            /**
             * 设置 Agent 服务端超时时间 / Set Agent server timeout
             * @param ms 超时时间(毫秒) / Timeout in milliseconds
             */
            set timeout(ms: Uint64)
        }
    }
}

export {}
