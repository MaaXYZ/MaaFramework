const console = {
    log: v => {
        print(`${v}`)
    }
}

async function main() {
    print('1')
    const devs = await maa.AdbController.find()
    print('1.5')
    const dev = devs[0]
    print('2')
    dev.shift()
    print('3')
    console.log(dev)
    print('4')
    console.log(new maa.AdbController(...dev))
    print('5')
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
