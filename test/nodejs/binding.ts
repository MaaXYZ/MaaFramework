import * as maa from './maa'

const myReco: maa.CustomRecognizerCallback = async self => {
    console.log('on myReco')
    const entry = 'ColorMatch'
    const ppover = {
        ColorMatch: {
            recognition: 'ColorMatch',
            lower: [100, 100, 100],
            upper: [255, 255, 255],
            action: 'Click'
        }
    }

    await self.context.run_pipeline(entry, ppover)
    await self.context.run_action(
        entry,
        {
            x: 114,
            y: 514,
            width: 191,
            height: 810
        },
        'RunAction Detail',
        ppover
    )
    const reco_detail = await self.context.run_recognition(entry, self.image, ppover)
    console.log('reco_detail:', reco_detail)

    return [{ x: 11, y: 4, width: 5, height: 14 }, 'Hello World!']
}

const myAct: maa.CustomActionCallback = async self => {
    console.log('on myAct')
    const ctrl = self.context.tasker.controller
    if (!ctrl) {
        return false
    }
    await ctrl.post_screencap().wait()
    const new_image = ctrl.cached_image
    await ctrl.post_click(191, 98).wait()
    await ctrl.post_swipe(100, 200, 300, 400, 100).wait()
    await ctrl.post_input_text('Hello World!').wait()
    await ctrl.post_press_key(32).wait()
    await ctrl.post_touch_down(1, 100, 100, 0).wait()
    await ctrl.post_touch_move(1, 200, 200, 0).wait()
    await ctrl.post_touch_up(1).wait()
    return true
}

async function main() {
    console.log('MaaFw Version:', maa.Global.version)

    maa.Global.config_init_option('../../install/bin')

    const res = new maa.Resource()
    const ctrl = new maa.DbgController(
        '../../install/test/PipelineSmoking/Screenshot',
        '../../install/test/user',
        maa.api.DbgControllerType.CarouselImage,
        '{}'
    )
    await ctrl.post_connection().wait()

    const tskr = new maa.Tasker()
    tskr.bind(res)
    tskr.bind(ctrl)

    if (!tskr.inited) {
        return 1
    }

    res.register_custom_recognizer('MyRec', myReco)
    res.register_custom_action('MyAct', myAct)

    const ppover = {
        Entry: { next: 'Rec' },
        Rec: {
            recognition: 'Custom',
            custom_recognition: 'MyRec',
            action: 'Custom',
            custom_action: 'MyAct',
            custom_action_param: '👋哈哈哈(*´▽｀)ノノ😀'
        }
    }

    let detail = await tskr.post_pipeline('Entry', ppover).wait().get()
    if (!detail) {
        console.log('pipeline failed')
        return 1
    }
    console.log('pipeline detail:', detail)

    return 0
}

main().then(ec => {
    process.exit(ec)
})
