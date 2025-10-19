const console = {
    log: v => {
        print(`${v}`)
    }
}

async function main() {
    const dev = (await maa.AdbController.find())[0]
    dev.shift()
    const ctrl = new maa.AdbController(...dev)
    await ctrl.post_connection().wait()

    const res = new maa.Resource()
    await res.post_bundle('sample/resource').wait()

    const tsk = new maa.Tasker()

    tsk.resource = res
    tsk.controller = ctrl

    print(`inited: ${tsk.inited}`)

    print(`${ctrl} ${res} ${tsk}`)

    const imageJob = ctrl.post_screencap()
    print(`${imageJob} ${tsk.controller}`)

    return '123123'
}

main()
    .then(exit)
    .catch(err => {
        exit(`${err}`)
    })
