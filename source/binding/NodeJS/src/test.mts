async function main() {
    const imgJob = new maa.ImageJob(new maa.Controller('0'), '0')

    exit('123')
    /*
    const res = new maa.Resource()

    print('before await')
    const job = res.post_bundle('sample/resource')
    await job.wait().status
    print('after await')

    print(`loaded: ${res.loaded}\nhash: ${res.hash}\nnode_list: ${res.node_list?.join(', ')}`)

    exit('123')
    */
}

main().catch(err => {
    print(`${err}`)
    exit(`${err}`)
})
