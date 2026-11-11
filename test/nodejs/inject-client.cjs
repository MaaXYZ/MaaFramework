const { join } = require('node:path')

globalThis.maa = require(
    process.env.MAAFW_BINARY_PATH
        ? join(process.env.MAAFW_BINARY_PATH, 'MaaNode.node')
        : '../../install/bin/MaaNode.node'
)
