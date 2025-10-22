const console = {
    log: v => {
        print(`${v}`)
    }
}

async function main() {
    print(`${JSON.stringify(maa, null, 4)}`)

    const dev = (await maa.AdbController.find())[0]
    dev.shift()
    const ctrl = new maa.AdbController(...dev)

    ctrl.add_sink((ctrl, msg) => {
        print(`${ctrl} ${JSON.stringify(msg, null, 4)}`)
    })

    await ctrl.post_connection().wait()

    const res = new maa.Resource()
    await res.post_bundle('sample/resource').wait()

    const tsk = new maa.Tasker()

    tsk.resource = res
    tsk.controller = ctrl

    print(`inited: ${tsk.inited}`)

    return '123123'
}

main()
    .then(exit)
    .catch(err => {
        exit(`${err}`)
    })
