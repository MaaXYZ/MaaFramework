import { Global, Resource, Job } from '@nekosu/maa'
console.log(Global.version, Global.version_from_macro)

const res = new Resource()
console.log(res, res.status('0'))
const job = new Job(res, '0')
console.log(job, job.status)
