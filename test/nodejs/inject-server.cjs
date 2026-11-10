const { join } = require('node:path')

globalThis.maa = require(
    process.env.MAAFW_BINARY_PATH
        ? join(process.env.MAAFW_BINARY_PATH, 'MaaNodeServer.node')
        : '../../install/bin/MaaNodeServer.node'
)
