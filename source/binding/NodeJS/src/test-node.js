const maa = require('../../../../build/bin/Debug/MaaNodeNew.node')

async function main() {
    // const imgJob = new maa.ImageJob(new maa.Controller('0'), '0')

    // console.log(`${imgJob instanceof maa.Job}`)
    // console.log(`${imgJob instanceof maa.ImageJob}`)

    // const res = new maa.Resource()
    // const job = await res.post_bundle('sample/resource')
    // console.log(job, job.wait)
    // await job.wait().status

    console.log(maa.AdbController)
    console.log(new maa.AdbController())
}

main()
