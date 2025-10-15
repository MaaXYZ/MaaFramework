print('eval start')

async function main() {
    print('main start')
    const res = new maa.Resource()
    const job = res.post_bundle('sample/resource')

    print('before await')
    // const status = await res.wait(job.id)
    // print(`${status}`)
    print(`${job.wait}`)
    const pro = job.wait()
    const result = await pro.status
    print(`${result}`)
    print('after await')

    exit('123')
    print('main end')

    globalThis.job = job
}

async function main2() {
    const res = new maa.Resource()
    const job = new maa.Job(res, '0')
    await 0
    exit('123')
}

main().catch(err => {
    print(`${err}`)
    exit(`${err}`)
})

print('eval end')
