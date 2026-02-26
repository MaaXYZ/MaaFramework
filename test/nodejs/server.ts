import './maa-server'

async function main() {
    console.log('MaaFw Version:', maa.Global)
    // console.log('MaaFw Role', maa.AgentRole)

    console.log('AgentServer', maa.Server)

    try {
        maa.Global.config_init_option('.') // This should failed
        process.exit(1)
    } catch {}

    process.exit(0)
}

main()
