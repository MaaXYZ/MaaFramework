import './maa-server'

async function main() {
    console.log('MaaFw Version:', maa.Global)
    // console.log('MaaFw Role', maa.AgentRole)

    console.log('AgentServer', maa.Server)

    process.exit(0)
}

main()
