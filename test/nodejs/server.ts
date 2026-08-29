// 依赖 Node >= 23.6 的原生 TypeScript 类型剥离（CI 已钉 Node 24，与 binding.ts 同机制）
import './maa-server.ts'

async function main() {
    console.log('MaaFw Version:', maa.Global.version)

    // Server 构建冒烟：maa.Server 存在且关键 API 齐全
    const server = maa.Server
    if (!server) {
        console.error('maa.Server is undefined')
        process.exit(1)
    }
    for (const api of [
        'register_custom_recognition',
        'register_custom_action',
        'add_resource_sink',
        'add_controller_sink',
        'add_tasker_sink',
        'add_context_sink',
        'set_shutdown_callback',
        'start_up',
        'shut_down',
        'join',
        'detach'
    ]) {
        if (typeof (server as unknown as Record<string, unknown>)[api] !== 'function') {
            console.error(`maa.Server.${api} is missing`)
            process.exit(1)
        }
    }
    console.log('maa.Server API surface OK')

    // set_shutdown_callback 冒烟：可注册、可覆盖注册（后注册者替换先注册者）
    server.set_shutdown_callback(() => {
        console.log('on shutdown callback (first, should be overwritten)')
    })

    server.set_shutdown_callback(async () => {
        console.log('on shutdown callback')
        // async 回调会被服务端阻塞等待完成（MaybePromise 语义）
        await new Promise(resolve => setTimeout(resolve, 100))
    })
    console.log('set_shutdown_callback registered (sync + async overwrite) OK')

    console.log('server.ts smoke tests passed')
    process.exit(0)
}

main()
