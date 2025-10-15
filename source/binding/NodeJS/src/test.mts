import { Global, Resource, Job } from '@nekosu/maa'
import * as os from 'qjs:os'

const res = new Resource()
const job = res.post_bundle('sample/resource')
console.log(job, job.status)

os.setTimeout(() => {
    console.log('wait finish!')
    gc()
}, 2000)
