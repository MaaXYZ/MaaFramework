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
    const img = await ctrl.post_screencap().wait().get()
    save('1.png', img)
    /*
    const res = new maa.Resource()

    print('before await')
    const job = res.post_bundle('sample/resource')
    await job.wait().status
    print('after await')

    print(`loaded: ${res.loaded}\nhash: ${res.hash}\nnode_list: ${res.node_list?.join(', ')}`)

    exit('123')
    */

    return '123123'
}

main()
    .then(exit)
    .catch(err => {
        print(`${err}`)
        exit(`${err}`)
    })
