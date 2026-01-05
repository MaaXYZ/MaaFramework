import '../../test/nodejs/maa-client'
// import '@maaxyz/maa-node'

console.log(maa.Global.version)

const my_reco: maa.CustomRecognitionCallback = async self => {
    let reco_detail = await self.context.run_recognition('MyCustomOCR', self.image, {
        MyCustomOCR: {
            roi: [100, 100, 200, 300]
        }
    })

    self.context.override_pipeline({
        MyCustomOCR: {
            roi: [1, 1, 114, 514]
        }
    })

    const new_context = self.context.clone()
    new_context.override_pipeline({
        MyCustomOCR: {
            roi: [100, 200, 300, 400]
        }
    })

    reco_detail = await new_context.run_recognition('MyCustomOCR', self.image)

    const click_job = self.context.tasker.controller!.post_click(10, 20)
    await click_job.wait()

    self.context.override_next(self.task, ['TaskA', 'TaskB'])

    return [[0, 0, 100, 100], 'Hello World!']
}

async function main() {
    maa.Global.config_init_option('./')

    const res = new maa.Resource()
    res.add_sink((_, msg) => {
        console.log(msg)
    })
    await res.post_bundle('sample/resource').wait()

    const devices = await maa.AdbController.find()
    if (!devices || devices.length === 0) {
        return
    }
    const [name, adb_path, address, screencap_methods, input_methods, config] = devices[0]
    const ctrl = new maa.AdbController(adb_path, address, screencap_methods, input_methods, config)
    ctrl.add_sink((_, msg) => {
        console.log(msg)
    })
    await ctrl.post_connection().wait()

    const tskr = new maa.Tasker()
    tskr.add_sink((_, msg) => {
        console.log(msg)
    })
    tskr.add_context_sink((_, msg) => {
        console.log(msg)
    })

    tskr.controller = ctrl
    tskr.resource = res

    console.log(tskr.inited)

    res.register_custom_recognizer('MyRec', my_reco)

    let task_detail = await tskr.post_task('StartUpAndClickButton').wait().get()

    tskr.destroy()
    ctrl.destroy()
    res.destroy()
}

main()
