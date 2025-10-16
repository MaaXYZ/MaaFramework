async function main() {
    const res = new maa.Resource()

    print('before await')
    const job = res.post_bundle('sample/resource')
    await job.wait().status
    print('after await')

    exit('123')
}

main().catch(err => {
    print(`${err}`)
    exit(`${err}`)
})
