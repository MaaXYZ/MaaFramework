declare module globalThis {
    let MaaAPI: any
}

globalThis.MaaAPI = require('../../install/binding/NodeJS/bin/MaaNode.node')
