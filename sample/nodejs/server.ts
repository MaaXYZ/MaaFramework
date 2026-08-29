import '../../test/nodejs/maa-server'
// import '@maaxyz/maa-node/server'

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

    const socket_id = process.argv[process.argv.length - 1]

    maa.Server.set_shutdown_callback(async () => {
        console.log('on_shutdown: cleanup before server stops...')
    })

    await maa.Server.start_up(socket_id)

    maa.Server.register_custom_recognition('MyRecongition', my_reco)

    await maa.Server.join()
    maa.Server.shut_down()
}

main()
