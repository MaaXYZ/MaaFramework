const maa = require('../../../../build/bin/Debug/MaaNodeNew.node')

async function main() {
    const res = new maa.Resource()
    const job = await res.post_bundle('sample/resource')
    console.log(job, job.wait)
    await job.wait().status
}

main()
