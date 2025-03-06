if (!globalThis.MaaAPI) {
    switch (`${process.platform}-${process.arch}`) {
        case 'win32-x64':
            globalThis.MaaAPI = require('@maaxyz/maa-node-win32-x64')
            break
        case 'linux-x64':
            globalThis.MaaAPI = require('@maaxyz/maa-node-linux-x64')
            break
        case 'linux-arm64':
            globalThis.MaaAPI = require('@maaxyz/maa-node-linux-arm64')
            break
        case 'darwin-x64':
            globalThis.MaaAPI = require('@maaxyz/maa-node-darwin-x64')
            break
        case 'darwin-arm64':
            globalThis.MaaAPI = require('@maaxyz/maa-node-darwin-arm64')
            break
        default:
            globalThis.MaaAPI = {}
            break
    }
}

module.exports = globalThis.MaaAPI
