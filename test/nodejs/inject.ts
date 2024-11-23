declare module globalThis {
    let MaaAPI: any
}

globalThis.MaaAPI = require('../../install/bin/MaaNode.node')
