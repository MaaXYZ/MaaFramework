import { join } from 'path'
import * as maa from './maa-server'

import fs from 'fs/promises'

async function main() {
    console.log('MaaFw Version:', maa.Global.version)
    console.log('MaaFw Role', maa.api.AgentRole)

    console.log('AgentServer', maa.AgentServer)

    process.exit(0)
}

main()
