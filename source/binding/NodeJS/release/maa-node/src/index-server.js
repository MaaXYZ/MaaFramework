if (!globalThis.maa) {
    switch (`${process.platform}-${process.arch}`) {
        case 'win32-x64':
            globalThis.maa = require('@maaxyz/maa-node-win32-x64/server')
            break
        case 'win32-arm64':
            globalThis.maa = require('@maaxyz/maa-node-win32-arm64/server')
            break
        case 'linux-x64':
            globalThis.maa = require('@maaxyz/maa-node-linux-x64/server')
            break
        case 'linux-arm64':
            globalThis.maa = require('@maaxyz/maa-node-linux-arm64/server')
            break
        case 'darwin-x64':
            globalThis.maa = require('@maaxyz/maa-node-darwin-x64/server')
            break
        case 'darwin-arm64':
            globalThis.maa = require('@maaxyz/maa-node-darwin-arm64/server')
            break
        default:
            globalThis.maa = {}
            break
    }
}

module.exports = globalThis.maa
